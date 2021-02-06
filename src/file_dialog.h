#pragma once
#include <string>
#include <vector>
#include <optional>

#include <filesystem>

struct FileDialog {
	std::string title;

	std::filesystem::path path;

	char                  selected[512] = { };
	std::filesystem::path selected_path;

	struct Entry {
		std::filesystem::path path;
		std::string display;
	};

	std::vector<Entry> directories;
	std::vector<Entry> files;

	bool saving = true;

	FileDialog() { change_path("projects"); }

	void show(bool saving);
	bool render();

private:
	void change_path(std::filesystem::path const & new_path);
};
