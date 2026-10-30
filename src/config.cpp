#include "config.h"
#include "files.h"

#include <windows.h>
#include <cstring>
#include <cwctype>

namespace
{
	std::wstring Upper(std::wstring s)
	{
		for (auto& c : s)
			c = (wchar_t)std::towupper(c);
		return s;
	}

	std::wstring Trim(const std::wstring& s)
	{
		size_t first = s.find_first_not_of(L" \t");
		if (first == std::wstring::npos)
			return L"";
		size_t last = s.find_last_not_of(L" \t");
		return s.substr(first, last - first + 1);
	}

	bool Widen(UINT codePage, DWORD flags, const char* data, int size, std::wstring& out)
	{
		out.clear();
		if (size == 0)
			return true;
		int n = MultiByteToWideChar(codePage, flags, data, size, nullptr, 0);
		if (n <= 0)
			return false;
		out.resize(n);
		MultiByteToWideChar(codePage, flags, data, size, &out[0], n);
		return true;
	}

	std::wstring Decode(const std::string& bytes, std::vector<std::wstring>& notes)
	{
		const unsigned char* b = (const unsigned char*)bytes.data();
		size_t size = bytes.size();

		if (size >= 2 && b[0] == 0xFF && b[1] == 0xFE) {	// UTF-16 LE (Notepad's "Unicode")
			std::wstring out((size - 2) / 2, L'\0');
			if (!out.empty())
				memcpy(&out[0], b + 2, out.size() * sizeof(wchar_t));
			return out;
		}
		if (size >= 2 && b[0] == 0xFE && b[1] == 0xFF) {	// UTF-16 BE
			std::wstring out;
			for (size_t i = 2; i + 1 < size; i += 2)
				out += (wchar_t)((b[i] << 8) | b[i + 1]);
			return out;
		}

		size_t skip = (size >= 3 && b[0] == 0xEF && b[1] == 0xBB && b[2] == 0xBF) ? 3 : 0;
		std::wstring out;
		if (Widen(CP_UTF8, MB_ERR_INVALID_CHARS, bytes.data() + skip, (int)(size - skip), out))
			return out;
		notes.push_back(L"gobinator.ini is not UTF-8, so it was read in the Windows code page.");
		Widen(CP_ACP, 0, bytes.data(), (int)size, out);
		return out;
	}

	std::vector<std::wstring> SplitLines(const std::wstring& text)
	{
		std::vector<std::wstring> lines;
		std::wstring line;
		for (size_t i = 0; i < text.size(); i++) {
			wchar_t c = text[i];
			if (c == L'\r' || c == L'\n') {
				lines.push_back(line);
				line.clear();
				if (c == L'\r' && i + 1 < text.size() && text[i + 1] == L'\n')
					i++;
			}
			else
				line += c;
		}
		if (!line.empty())
			lines.push_back(line);
		return lines;
	}

	bool IsComment(const std::wstring& trimmed)
	{
		return !trimmed.empty() && (trimmed[0] == L'#' || trimmed[0] == L';');
	}

	std::optional<std::wstring>* Slot(Config& config, const std::wstring& key)
	{
		if (key == L"JK_FOLDER") return &config.jkFolder;
		if (key == L"MOD_FOLDER") return &config.modFolder;
		if (key == L"JK_EXECUTABLE") return &config.jkExecutable;
		if (key == L"ADDITIONAL_LAUNCH_OPTIONS") return &config.launchOptions;
		return nullptr;
	}

	// v1.0's exclude list and JK Resurrection's BFP options turn up in old files. Both are ignored (docs/DECISIONS.md).
	bool IsRetired(const std::wstring& key)
	{
		return key == L"MOD_FOLDER_PURGE_EXCLUDE_FILE" || key.rfind(L"BFP_RANDOMIZE_", 0) == 0;
	}

	// A name v1.0's form takes as a key wherever it stands (ParseLegacy): one of the settings, or a retired one.
	bool IsKnownKey(const std::wstring& upper)
	{
		Config none;
		return Slot(none, upper) != nullptr || IsRetired(upper);
	}

	void Set(Config& config, const std::wstring& rawKey, std::wstring value, size_t lineNumber, bool& notedExclude)
	{
		std::wstring key = Upper(rawKey);
		std::wstring where = L"line " + std::to_wstring(lineNumber);
		if (IsRetired(key)) {
			if (key == L"MOD_FOLDER_PURGE_EXCLUDE_FILE" && !notedExclude) {
				config.notes.push_back(L"MOD_FOLDER_PURGE_EXCLUDE_FILE is no longer needed and was ignored: "
					L"Gobinator removes only the GOBs it copied in itself.");
				notedExclude = true;
			}
			return;
		}
		auto slot = Slot(config, key);
		if (!slot) {
			config.notes.push_back(where + L": unknown setting " + rawKey + L", ignored.");
			return;
		}
		if (slot->has_value())
			config.notes.push_back(where + L": " + key + L" is set again; this one counts.");
		if (value.empty() && key != L"ADDITIONAL_LAUNCH_OPTIONS")
			slot->reset();
		else
			*slot = value;
	}

	void ParseKeyValue(const std::vector<std::wstring>& lines, Config& config)
	{
		bool notedExclude = false;
		for (size_t i = 0; i < lines.size(); i++) {
			std::wstring line = Trim(lines[i]);
			if (line.empty() || IsComment(line) || line[0] == L'[')
				continue;
			size_t eq = line.find(L'=');
			if (eq == std::wstring::npos) {
				config.notes.push_back(L"line " + std::to_wstring(i + 1) + L" has no '=' and was ignored: " + line);
				continue;
			}
			std::wstring value = Trim(line.substr(eq + 1));
			if (value.size() >= 2 && value.front() == L'"' && value.back() == L'"')
				value = value.substr(1, value.size() - 2);
			Set(config, Trim(line.substr(0, eq)), value, i + 1, notedExclude);
		}
	}

	// v1.0's form: a key on one line, its value on the next, # comments anywhere. v1.0 counted blank lines too, so one
	// blank line or a BOM shifted every pair after it (docs/OVERVIEW.md, defect 3). Here blank lines are skipped, and a
	// known key name where a value should be means the key before it had an empty value.
	void ParseLegacy(const std::vector<std::wstring>& lines, Config& config)
	{
		bool notedExclude = false;
		std::wstring key;
		size_t keyLine = 0;
		for (size_t i = 0; i < lines.size(); i++) {
			std::wstring line = Trim(lines[i]);
			if (line.empty() || IsComment(line))
				continue;
			if (key.empty()) {
				key = line;
				keyLine = i + 1;
				continue;
			}
			std::wstring upper = Upper(line);
			if (Slot(config, upper) || IsRetired(upper)) {
				config.notes.push_back(L"line " + std::to_wstring(keyLine) + L": " + key +
					L" has no value line, so it was read as empty.");
				Set(config, key, L"", keyLine, notedExclude);
				key = line;
				keyLine = i + 1;
				continue;
			}
			Set(config, key, line, keyLine, notedExclude);
			key.clear();
		}
		if (!key.empty()) {
			config.notes.push_back(L"line " + std::to_wstring(keyLine) + L": " + key +
				L" has no value line, so it was read as empty.");
			Set(config, key, L"", keyLine, notedExclude);
		}
	}

	void Validate(Config& config)
	{
		if (!config.modFolder)
			return;
		const std::wstring& mod = *config.modFolder;
		if (mod.find_first_of(L" \t") != std::wstring::npos)
			config.errors.push_back(L"MOD_FOLDER (\"" + mod + L"\") contains a space. JK reads -path only up to the "
				L"first space, so use a folder name without spaces.");
		bool absolute = mod.find(L':') != std::wstring::npos || mod[0] == L'\\' || mod[0] == L'/';
		bool climbs = (L"\\" + mod + L"\\").find(L"\\..\\") != std::wstring::npos ||
			(L"/" + mod + L"/").find(L"/../") != std::wstring::npos;
		if (absolute || climbs)
			config.errors.push_back(L"MOD_FOLDER (\"" + mod + L"\") must be a folder inside the JK folder, "
				L"such as mod.");
	}
}

Config ParseConfig(const std::string& bytes)
{
	Config config;
	std::vector<std::wstring> lines = SplitLines(Decode(bytes, config.notes));

	// The first line that is not blank or a comment decides the form: a key = value line, or a lone key name.
	for (const auto& raw : lines) {
		std::wstring line = Trim(raw);
		if (line.empty() || IsComment(line) || line[0] == L'[')
			continue;
		config.legacyForm = line.find(L'=') == std::wstring::npos;
		break;
	}

	if (config.legacyForm)
		ParseLegacy(lines, config);
	else
		ParseKeyValue(lines, config);
	Validate(config);
	return config;
}

std::wstring ResolveJkFolder(const Config& config, const std::wstring& ownFolder)
{
	if (!config.jkFolder)
		return ownFolder;
	return FullPath(JoinPath(ownFolder, *config.jkFolder));
}

std::string SetIniValue(const std::string& bytes, const std::wstring& key, const std::wstring& value)
{
	std::vector<std::wstring> notes;
	std::wstring text = Decode(bytes, notes);
	std::wstring eol = text.find(L"\r\n") != std::wstring::npos || text.find(L'\n') == std::wstring::npos ? L"\r\n" : L"\n";
	std::vector<std::wstring> lines = SplitLines(text);
	std::wstring upperKey = Upper(key);

	// The form, as ParseConfig decides it: the first line that is not blank or a comment.
	bool legacy = false;
	for (const auto& raw : lines) {
		std::wstring line = Trim(raw);
		if (line.empty() || IsComment(line) || line[0] == L'[')
			continue;
		legacy = line.find(L'=') == std::wstring::npos;
		break;
	}

	if (legacy) {
		// Walk as ParseLegacy does: a key, then its value on the next line that is not blank or a comment, unless
		// that line is itself a known key. The last line of the key counts, as the last setting does.
		size_t keyAt = std::wstring::npos;
		bool expectKey = true;
		for (size_t i = 0; i < lines.size(); i++) {
			std::wstring line = Trim(lines[i]);
			if (line.empty() || IsComment(line))
				continue;
			std::wstring upper = Upper(line);
			if (expectKey || IsKnownKey(upper)) {
				if (upper == upperKey)
					keyAt = i;
				expectKey = false;
				continue;
			}
			expectKey = true;
		}
		if (keyAt == std::wstring::npos) {
			lines.push_back(key);
			lines.push_back(value);
		}
		else {
			size_t valueAt = keyAt + 1;
			while (valueAt < lines.size()) {
				std::wstring line = Trim(lines[valueAt]);
				if (!line.empty() && !IsComment(line))
					break;
				valueAt++;
			}
			if (valueAt < lines.size() && !IsKnownKey(Upper(Trim(lines[valueAt]))))
				lines[valueAt] = value;
			else
				lines.insert(lines.begin() + keyAt + 1, value);
		}
	}
	else {
		size_t active = std::wstring::npos, example = std::wstring::npos;
		for (size_t i = 0; i < lines.size(); i++) {
			std::wstring line = Trim(lines[i]);
			if (line.empty())
				continue;
			bool comment = IsComment(line);
			std::wstring body = comment ? Trim(line.substr(1)) : line;
			size_t eq = body.find(L'=');
			if (eq == std::wstring::npos || Upper(Trim(body.substr(0, eq))) != upperKey)
				continue;
			if (comment) {
				if (example == std::wstring::npos)
					example = i;
			}
			else
				active = i;
		}
		std::wstring setting = key + L" = " + value;
		if (active != std::wstring::npos)
			lines[active] = setting;
		else if (example != std::wstring::npos)
			lines[example] = setting;
		else {
			if (!lines.empty())
				lines.push_back(L"");
			lines.push_back(setting);
		}
	}

	std::wstring out;
	for (const auto& line : lines)
		out += line + eol;
	return "\xEF\xBB\xBF" + ToUtf8(out);
}
