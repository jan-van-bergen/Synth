#include "file_dialog.h"

#include <filesystem>

#include <ImGui/imgui.h>

#include <SDL2/SDL_scancode.h>

FileDialog::FileDialog() { 
	change_path("projects");
}

void FileDialog::show(Type type, std::string const & title, std::string const & default_path, std::string const & file_extension, OnFinishedCallBack callback) {
	this->type  = type;
	this->file_extension = file_extension;
	this->title = title + "###FileDialog";
	this->callback = callback;

	strcpy_s(selected, file_extension_history[file_extension].c_str());

	change_path(default_path);

	should_open = true;
}

void FileDialog::render() {
	if (should_open) {
		should_open = false;

		ImGui::OpenPopup(title.c_str());
	}

	if (ImGui::BeginPopupModal(title.c_str())) {
		if (ImGui::Button("^")) {
			change_path(std::filesystem::path(path).parent_path().string());
		}
		ImGui::SameLine();
		ImGui::Text(path.c_str());
		ImGui::Separator();

		auto avail = ImGui::GetContentRegionAvail();

		ImGui::BeginChild("Browser", ImVec2(avail.x, avail.y - 2 * ImGui::GetTextLineHeightWithSpacing() - 12));

		std::string const * new_path = nullptr;

		char label[128];
		
		// Display directories
		for (auto const & directory : directories) {
			sprintf_s(label, "[D] %s", directory.display.c_str());
			if (ImGui::Selectable(label)) {
				new_path = &directory.path;
			}
		}

		ImGui::Separator();
		
		auto confirmed = false;

		// Display files
		for (auto const & file : files) {
			sprintf_s(label, "[F] %s", file.display.c_str());
			if (ImGui::Selectable(label)) {
				strcpy_s(selected, file.display.c_str());
			}

			if (ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left)) {
				confirmed = true;
			}
		}

		if (new_path) change_path(*new_path);

		ImGui::EndChild();

		// Display selected file, save, cancel
		ImGui::SetNextItemWidth(avail.x);
		ImGui::InputText("", selected, sizeof(selected));

		char const * button_text;
		switch (type) {
			case Type::SAVE: button_text = "Save"; break;
			case Type::OPEN: button_text = "Open"; break;
			default: abort();
		}

		if (ImGui::Button(button_text) || ImGui::IsKeyPressed(SDL_SCANCODE_RETURN)) confirmed = true;

		ImGui::SameLine();
		if (ImGui::Button("Cancel")) ImGui::CloseCurrentPopup();

		if (type == Type::SAVE) {
			if (confirmed) {
				auto has_extension = strchr(selected, '.') != nullptr;
				if (!has_extension) {
					strcat_s(selected, ".json");
				}

				selected_path = path + '/' + selected;

				if (std::filesystem::exists(selected_path)) {
					ImGui::OpenPopup("Overwrite");
					confirmed = false;
				}
			}

			if (ImGui::BeginPopupModal("Overwrite")) {
				ImGui::Text("File '%s' already exists. Do you want to overwrite?", selected);

				if (ImGui::Button("Yes") || ImGui::IsKeyPressed(SDL_SCANCODE_RETURN)) {
					ImGui::CloseCurrentPopup();
					confirmed = true;
				}

				ImGui::SameLine();
				if (ImGui::Button("No")) {
					ImGui::CloseCurrentPopup();
					confirmed = false;
				}

				ImGui::EndPopup();
			}
		} else if (confirmed) {	
			selected_path = path + '/' + selected;
		}

		if (confirmed || ImGui::IsKeyPressed(SDL_SCANCODE_ESCAPE)) ImGui::CloseCurrentPopup(); 

		ImGui::EndPopup();
		
		if (confirmed) {
			file_extension_history[file_extension] = std::string(selected);

			callback(selected_path.c_str());
		}
	}
}

void FileDialog::change_path(std::string const & new_path) {
	std::filesystem::path absolute_path = 
		std::filesystem::exists(new_path) ?
		std::filesystem::absolute(new_path) :
		std::filesystem::absolute(".");

	path = absolute_path.string();

	directories.clear();
	files      .clear();

	for (auto const & item : std::filesystem::directory_iterator(absolute_path, std::filesystem::directory_options::skip_permission_denied)) {
		try {
			auto relative_path = std::filesystem::relative(item.path(), absolute_path).string();

			if (item.is_directory()) {
				directories.emplace_back(item.path().string(), std::move(relative_path));
			} else {
				files.emplace_back(item.path().string(), std::move(relative_path));
			}
		} catch (std::filesystem::filesystem_error const & err) { }
	}
}
