#pragma once
#include <string>
#include <vector>
#include <optional>
#include <functional>

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

	enum struct Type {
		SAVE,
		OPEN
	} type;

	bool should_open = false;

	using OnFinishedCallBack = std::function<void (char const *)>;

	OnFinishedCallBack callback;

	FileDialog() { 
		if (std::filesystem::exists("projects/")) {
			change_path("projects");
		} else {
			change_path(".");
		}
	}

	void show(Type type, std::string const & title, std::string const & default_path, OnFinishedCallBack callback);
	void render();

private:
	void change_path(std::filesystem::path const & new_path);
};
