#include "tic_utils.h"
#include <cstdlib>
#include <set>

std::string sanitize_ascii_printable(const std::string& val) {
	std::string out;
	for (char c : val) {
		if (c >= 32 && c <= 126)
			out += c;
	}
	return out;
}

std::string get_env(const char* var, const char* def) {
	const char* val = std::getenv(var);
	std::string s = val ? std::string(val) : std::string(def);
	return sanitize_ascii_printable(s);
}

unsigned long get_env_ulong(const char* var, unsigned long def) {
	const char* val = std::getenv(var);
	if (val) {
		try { return std::stoul(val); } catch (...) { return def; }
	}
	return def;
}

std::string sanitize_label(const std::string& label) {
	std::string out;
	for (char c : label) {
		if ((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') || (c >= '0' && c <= '9') || c == '_' || c == '-')
			out += c;
		else
			out += '_';
	}
	return out;
}

std::string sanitize_value(const std::string& value) {
	return sanitize_ascii_printable(value);
}

