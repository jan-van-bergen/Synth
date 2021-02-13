#include "file_dialog.h"

#include <ImGui/imgui.h>

#include <SDL2/SDL_scancode.h>

void FileDialog::show(bool saving) {
	this->saving = saving;
	if (saving) {
		title = "Save File###FileDialog";
	} else {
		title = "Open File###FileDialog";
	}

	ImGui::OpenPopup(title.c_str());

	change_path(path); // Refresh
}

bool FileDialog::render() {
	if (ImGui::BeginPopupModal(title.c_str())) {
		if (ImGui::Button("^")) {
			change_path(std::filesystem::path(path).parent_path().string());
		}
		ImGui::SameLine();
		ImGui::Text(path.string().c_str());
		ImGui::Separator();

		auto avail = ImGui::GetContentRegionAvail();

		ImGui::BeginChild("Browser", ImVec2(avail.x, avail.y - 2 * ImGui::GetTextLineHeightWithSpacing() - 12));

		std::filesystem::path const * new_path = nullptr;

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

		if (ImGui::Button(saving ? "Save" : "Open")) confirmed = true;

		ImGui::SameLine();
		if (ImGui::Button("Cancel")) ImGui::CloseCurrentPopup();

		if (saving) {
			if (confirmed) {
				auto has_extension = strchr(selected, '.') != nullptr;
				if (!has_extension) {
					strcat_s(selected, ".json");
				}

				selected_path = path / selected;

				if (std::filesystem::exists(selected_path)) {
					ImGui::OpenPopup("Overwrite");
					confirmed = false;
				}
			}

			if (ImGui::BeginPopupModal("Overwrite")) {
				ImGui::Text("File '%s' already exists. Do you want to overwrite?", selected);

				if (ImGui::Button("Yes")) {
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
			selected_path = path / selected;
		}

		if (confirmed || ImGui::IsKeyPressed(SDL_SCANCODE_ESCAPE)) ImGui::CloseCurrentPopup(); 

		ImGui::EndPopup();

		return confirmed;
	}
}

void FileDialog::change_path(std::filesystem::path const & new_path) {
	path = std::filesystem::absolute(new_path);

	directories.clear();
	files      .clear();

	for (auto const & item : std::filesystem::directory_iterator(path, std::filesystem::directory_options::skip_permission_denied)) {
		try {
			auto relative_path = std::filesystem::relative(item.path(), path).string();

			if (item.is_directory()) {
				directories.emplace_back(item.path(), std::move(relative_path));
			} else {
				files.emplace_back(item.path(), std::move(relative_path));
			}
		} catch (std::filesystem::filesystem_error const & err) { }
	}
}
