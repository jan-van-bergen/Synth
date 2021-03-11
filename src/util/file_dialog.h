#pragma once
#include <string>
#include <vector>
#include <optional>
#include <functional>

struct FileDialog {
	enum struct Type {
		SAVE,
		OPEN
	};

private:
	std::string title;

	std::string path;

	std::string                                  file_extension;
	std::unordered_map<std::string, std::string> file_extension_history; // For every file extension, what was the last filename used

	char        selected[512] = { };
	std::string selected_path;

	struct Entry {
		std::string path;
		std::string display;
	};

	std::vector<Entry> directories;
	std::vector<Entry> files;

	Type type;

	bool should_open = false;

	using OnFinishedCallBack = std::function<void (char const *)>;

	OnFinishedCallBack callback;

public:
	FileDialog();

	void show(Type type, std::string const & title, std::string const & default_path, std::string const & file_extension, OnFinishedCallBack callback);
	void render();

private:
	void change_path(std::string const & new_path);
};
