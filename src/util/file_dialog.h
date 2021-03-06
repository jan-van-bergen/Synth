#pragma once
#include <string>
#include <vector>
#include <optional>
#include <functional>

#include <filesystem>

struct FileDialog {
	enum struct Type {
		SAVE,
		OPEN
	};

private:
	std::string title;

	std::filesystem::path path;

	std::string                                  file_extension;
	std::unordered_map<std::string, std::string> file_extension_history; // For every file extension, what was the last filename used

	char                  selected[512] = { };
	std::filesystem::path selected_path;

	struct Entry {
		std::filesystem::path path;
		std::string display;
	};

	std::vector<Entry> directories;
	std::vector<Entry> files;

	Type type;

	bool should_open = false;

	using OnFinishedCallBack = std::function<void (char const *)>;

	OnFinishedCallBack callback;

public:
	FileDialog() { 
		if (std::filesystem::exists("projects/")) {
			change_path("projects");
		} else {
			change_path(".");
		}
	}

	void show(Type type, std::string const & title, std::string const & default_path, std::string const & file_extension, OnFinishedCallBack callback);
	void render();

private:
	void change_path(std::filesystem::path const & new_path);
};
