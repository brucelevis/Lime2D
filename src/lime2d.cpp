/*!
 * \file lime2d.cpp
 * \author Mark Guerra
 * \date 6/9/2016
 * \copyright 2016 Limeoats
 */


#include <sstream>
#include <fstream>
#include <iostream>
#include <algorithm>

#include "lime2d.h"

#include "../libext/imgui.h"
#include "../libext/imgui-SFML.h"
#include "../libext/imgui_internal.h"

/*******************
 *  Lime2D Editor  *
 *******************/

l2d::Editor::Editor(bool enabled, sf::RenderWindow* window) :
    _graphics(new l2d_internal::Graphics(window)),
    _level(this->_graphics, "l2dSTART"),
    _showGridLines(true),
    _tilesetEnabled(false),
    _eraserActive(false),
    _windowHasFocus(true)
{
    this->_enabled = enabled;
    ImGui::SFML::Init(*window);
    this->_window = window;

    if (!this->_ambientLight.loadFromFile("content/shaders/ambient.frag", sf::Shader::Fragment)) {
        return;
    }

}

void l2d::Editor::toggle() {
    this->_enabled = !this->_enabled;
}

void l2d::Editor::processEvent(sf::Event &event) {
    ImGui::SFML::ProcessEvent(event);
    if (event.type == sf::Event::GainedFocus) {
        this->_windowHasFocus = true;
    }
    if (event.type == sf::Event::LostFocus) {
        this->_windowHasFocus = false;
    }
    if (event.type == sf::Event::KeyReleased) {
        if (event.key.code == sf::Keyboard::T) {
            if (this->_level.getName() != "l2dSTART") {
                this->_tilesetEnabled = !this->_tilesetEnabled;
            }
        }
        else if (event.key.code == sf::Keyboard::G) {
            this->_showGridLines = !this->_showGridLines;
        }
        else if (event.key.code == sf::Keyboard::U) {
            this->_level.undo();
        }
        else if (event.key.code == sf::Keyboard::R) {
            this->_level.redo();
        }
    }
}

void l2d::Editor::render() {
    if (this->_enabled) {
        this->_ambientLight.setParameter("texture", sf::Shader::CurrentTexture);
        this->_ambientLight.setParameter("color", this->_level.getAmbientColor().r / 255.0f, this->_level.getAmbientColor().g / 255.0f, this->_level.getAmbientColor().b / 255.0f);
        this->_ambientLight.setParameter("intensity", this->_level.getAmbientIntensity());
        this->_level.draw(&this->_ambientLight);
        //Draw the grid lines if appropriate
        if (this->_level.getName() != "l2dSTART") {
            if (this->_showGridLines) {
                //Horizontal lines
                for (int i = 0; i < this->_level.getSize().y + 1; ++i) {
                    sf::Vertex line[] = {
                            sf::Vertex(sf::Vector2f(0, i * (this->_level.getTileSize().y * std::stof(
                                    l2d_internal::utils::getConfigValue("tile_scale_y"))))),
                            sf::Vertex(sf::Vector2f(this->_level.getSize().x * this->_level.getTileSize().x *
                                                    std::stof(l2d_internal::utils::getConfigValue("tile_scale_x")),
                                                    i * (this->_level.getTileSize().y *
                                                         std::stof(
                                                                 l2d_internal::utils::getConfigValue("tile_scale_y")))))
                    };
                    this->_graphics->draw(line, 2, sf::Lines);
                }
                //Vertical lines
                for (int i = 0; i < this->_level.getSize().x + 1; ++i) {
                    sf::Vertex line[] = {
                            sf::Vertex(sf::Vector2f(i * (this->_level.getTileSize().x * std::stof(
                                    l2d_internal::utils::getConfigValue("tile_scale_x"))), 0)),
                            sf::Vertex(sf::Vector2f(i * this->_level.getTileSize().x *
                                                    std::stof(l2d_internal::utils::getConfigValue("tile_scale_x")),
                                                    this->_level.getSize().y * (this->_level.getTileSize().y *
                                                                                std::stof(
                                                                                        l2d_internal::utils::getConfigValue(
                                                                                                "tile_scale_y")))))
                    };
                    this->_graphics->draw(line, 2, sf::Lines);
                }
                //Get the mouse position and draw a square or something around the correct grid tile
                sf::Vector2f mousePos(
                        sf::Mouse::getPosition(*this->_window).x + this->_graphics->getCamera()->getRect().left,
                        sf::Mouse::getPosition(*this->_window).y + this->_graphics->getCamera()->getRect().top);

                if (mousePos.x >= 0 && mousePos.x <= (this->_level.getSize().x * this->_level.getTileSize().x *
                                                      std::stof(l2d_internal::utils::getConfigValue("tile_scale_x"))) &&
                    mousePos.y >= 0 && mousePos.y <= (this->_level.getSize().y * this->_level.getTileSize().y *
                                                      std::stof(l2d_internal::utils::getConfigValue("tile_scale_y")))) {
                    sf::RectangleShape rectangle;
                    rectangle.setSize(sf::Vector2f(this->_level.getTileSize().x *
                                                   std::stof(l2d_internal::utils::getConfigValue("tile_scale_x")) - 1,
                                                   this->_level.getTileSize().y *
                                                   std::stof(l2d_internal::utils::getConfigValue("tile_scale_y")) - 1));
                    rectangle.setOutlineColor(this->_eraserActive ? sf::Color::Blue : sf::Color::Magenta);
                    rectangle.setOutlineThickness(2);
                    rectangle.setPosition((mousePos.x - ((int) mousePos.x % (int) (this->_level.getTileSize().x *
                                                                                   std::stof(
                                                                                           l2d_internal::utils::getConfigValue(
                                                                                                   "tile_scale_x"))))),
                                          mousePos.y - ((int) mousePos.y % (int) (this->_level.getTileSize().y *
                                                                                  std::stof(
                                                                                          l2d_internal::utils::getConfigValue(
                                                                                                  "tile_scale_y")))));
                    rectangle.setFillColor(sf::Color::Transparent);
                    this->_window->draw(rectangle);
                }
            }
        }
        sf::RectangleShape rectangle;
        rectangle.setSize(sf::Vector2f(this->_window->getSize().x, 30));
        rectangle.setFillColor(sf::Color::Black);
        rectangle.setPosition(0 + this->_graphics->getCamera()->getRect().left, this->_window->getSize().y - 30 + this->_graphics->getCamera()->getRect().top);
        this->_window->draw(rectangle);
        ImGui::Render();
    }
}

void l2d::Editor::update(sf::Time t) {
    if (this->_enabled) {
        ImGui::SFML::Update(t);


        /*
         *  Menu
         *  File, View, Map, Animation, Help
         */
        static bool cbMapEditor = false;
        static bool cbAnimationEditor = false;
        static bool cbShowGridLines = true;
        static bool aboutBoxVisible = false;
        static bool mapSelectBoxVisible = false;
        static bool configWindowVisible = false;
        static bool tilePropertiesWindowVisible = false;
        static bool newMapBoxVisible = false;
        static bool newMapExistsOverwriteVisible = false;
        static bool tilesetWindowVisible = false;
        static bool lightEditorWindowVisible = false;
        static bool mainHasFocus = true;

        static sf::Vector2f mousePos(0.0f, 0.0f);

        static std::string currentFeature = "Lime2D";
        static std::string currentStatus = "";
        static bool showCurrentStatus = false;
        static int currentStatusTimer = 0;

        static std::vector<std::shared_ptr<sf::Texture>> tilesets;

        static int mapSelectIndex = 0;

        static bool showSpecificTileProperties = false;
        static std::shared_ptr<l2d_internal::Tile> showSpecificTilePropertiesTile = nullptr;
        static std::shared_ptr<l2d_internal::Layer> showSpecificTilePropertiesLayer = nullptr;


        //Drawing tiles variables
        static bool tileHasBeenSelected = false;
        static std::string selectedTilesetPath = "content/tilesets/outside.png";
        static sf::Vector2i selectedTileSrcPos(0,0);
        static int selectedTileLayer = 1;
        static sf::Vector2i selectedTilesetSize(0,0);

        //Light variables

        static l2d_internal::LightType selectedLightType = l2d_internal::LightType::None;


        //startStatusTimer function is in written like this so that it can exist within the update function
        //This way, it can access the static timer variables without making them member variables
        static auto startStatusTimer = [&](std::string newStatus, int time) {
            currentStatus = newStatus;
            currentStatusTimer = time;
            showCurrentStatus = true;
        };


        //Set mainHasFocus (very important)
        //This tells Lime2D that it can draw tiles to the screen. We don't want it drawing if other windows have focus.
        mainHasFocus = !(tilesetWindowVisible || newMapBoxVisible || tilePropertiesWindowVisible || configWindowVisible || mapSelectBoxVisible || aboutBoxVisible || lightEditorWindowVisible);

        cbShowGridLines = this->_showGridLines;


        //Config window
        if (configWindowVisible) {
            static bool loaded = false;

            ImGui::SetNextWindowPosCenter();
            ImGui::SetNextWindowSize(ImVec2(500, 400));
            static std::string configureMapErrorText = "";
            ImGui::Begin("Configure map editor", nullptr, ImVec2(500,400), 100.0f, ImGuiWindowFlags_AlwaysAutoResize);

            ImGui::PushID("ConfigureMapPath");
            ImGui::Text("Map path");
            static char mapPath[500] = "";
            if (l2d_internal::utils::getConfigValue("map_path") != "" && !loaded) {
                strcpy(mapPath, l2d_internal::utils::getConfigValue("map_path").c_str());
            }
            ImGui::PushItemWidth(300);
            ImGui::InputText("", mapPath, 500);
            ImGui::PopItemWidth();
            ImGui::Separator();
            ImGui::PopID();

            ImGui::PushID("ConfigureTilesetPath");
            ImGui::Text("Tileset path");
            static char tilesetPath[500] = "";
            if (l2d_internal::utils::getConfigValue("tileset_path") != "" && !loaded) {
                strcpy(tilesetPath, l2d_internal::utils::getConfigValue("tileset_path").c_str());
            }
            ImGui::PushItemWidth(300);
            ImGui::InputText("", tilesetPath, 500);
            ImGui::PopItemWidth();
            ImGui::Separator();
            ImGui::PopID();

            ImGui::PushItemWidth(100);

            ImGui::PushID("ConfigureSpriteScale");
            ImGui::Text("Sprite scale");
            static float spriteScaleX = l2d_internal::utils::getConfigValue("sprite_scale_x") == "" ? 1.0f : std::stof(l2d_internal::utils::getConfigValue("sprite_scale_x"));
            static float spriteScaleY = l2d_internal::utils::getConfigValue("sprite_scale_y") == "" ? 1.0f : std::stof(l2d_internal::utils::getConfigValue("sprite_scale_y"));
            ImGui::InputFloat("x", &spriteScaleX, 0.1f, 0.0f, 2);
            ImGui::InputFloat("y", &spriteScaleY, 0.1f, 0.0f, 2);
            ImGui::Separator();
            ImGui::PopID();

            ImGui::PushID("ConfigureTileScale");
            ImGui::Text("Tile scale");
            static float tileScaleX = l2d_internal::utils::getConfigValue("tile_scale_x") == "" ? 1.0f : std::stof(l2d_internal::utils::getConfigValue("tile_scale_x"));
            static float tileScaleY = l2d_internal::utils::getConfigValue("tile_scale_y") == "" ? 1.0f : std::stof(l2d_internal::utils::getConfigValue("tile_scale_y"));
            ImGui::InputFloat("x", &tileScaleX, 0.1f, 0.0f, 2);
            ImGui::InputFloat("y", &tileScaleY, 0.1f, 0.0f, 2);
            ImGui::Separator();
            ImGui::PopID();

            ImGui::PushID("ConfigureScreenSize");
            ImGui::Text("Screen size");
            static int screenSizeX = l2d_internal::utils::getConfigValue("screen_size_x") == "" ? 1 : std::stoi(l2d_internal::utils::getConfigValue("screen_size_x"));
            static int screenSizeY = l2d_internal::utils::getConfigValue("screen_size_y") == "" ? 1 : std::stoi(l2d_internal::utils::getConfigValue("screen_size_y"));
            ImGui::InputInt("x", &screenSizeX, 5);
            ImGui::InputInt("y", &screenSizeY, 5);
            ImGui::Separator();
            ImGui::PopID();

            ImGui::PopItemWidth();

            if (ImGui::Button("Save")) {
                //Lots of error checking on all of the values
                //Saving to lime2d.config
                if (strlen(mapPath) <= 0) {
                    configureMapErrorText = "You must enter the location of your maps!";
                }
                else if (strlen(tilesetPath) <= 0) {
                    configureMapErrorText = "You must enter the location of your tilesets!";
                }
                else if (spriteScaleX < 0 || spriteScaleY < 0) {
                    configureMapErrorText = "Sprite scale cannot be negative!";
                }
                else if (tileScaleX < 0 || tileScaleY < 0) {
                    configureMapErrorText = "Tile scale cannot be negative!";
                }
                else if (screenSizeX < 0 || screenSizeY < 0) {
                    configureMapErrorText = "Screen size cannot be negative!";
                }
                else {
                    configureMapErrorText = "";
                    //Everything checks out, so save.
                    std::ofstream os("lime2d.config");
                    if (os.is_open()) {
                        os << "map_path=" << mapPath << "\n";
                        os << "tileset_path=" << tilesetPath << "\n";
                        os << "sprite_scale_x=" << spriteScaleX << "\n";
                        os << "sprite_scale_y=" << spriteScaleY << "\n";
                        os << "tile_scale_x=" << tileScaleX << "\n";
                        os << "tile_scale_y=" << tileScaleY << "\n";
                        os << "screen_size_x=" << screenSizeX << "\n";
                        os << "screen_size_y=" << screenSizeY << "\n";
                        os.close();
                        if (this->_level.getName() != "l2dSTART") {
                            std::string name = this->_level.getName();
                            this->_level.loadMap(name);
                        }
                        configWindowVisible = false;
                        configureMapErrorText = "";
                    }
                    else {
                        configureMapErrorText = "Unable to save file. Please refer to www.limeoats.com/lime2d for more information.";
                    }
                }
            }
            ImGui::SameLine();
            if (ImGui::Button("Cancel")) {
                configWindowVisible = false;
                configureMapErrorText = "";
            }
            ImGui::Text(configureMapErrorText.c_str());
            ImGui::End();
            loaded = true;
        }

        //About box
        if (aboutBoxVisible) {
            ImGui::SetNextWindowSize(ImVec2(300, 140));
            ImGui::Begin("About Lime2D", nullptr, ImVec2(300, 200), 100.0f, ImGuiWindowFlags_AlwaysAutoResize);
            ImGui::Text("Lime2D Editor\n\nBy: Limeoats\nCopyright \u00a9 2016");
            ImGui::Separator();
            if (ImGui::Button("Close")) {
                aboutBoxVisible = false;
            }
            ImGui::End();
        }

        //Map select box
        if (mapSelectBoxVisible) {
            std::stringstream ss;
            ss << l2d_internal::utils::getConfigValue("map_path") << "*";
            std::vector<const char*> mapFiles = l2d_internal::utils::getFilesInDirectory(ss.str());
            ImGui::SetNextWindowPosCenter();
            ImGui::SetNextWindowSize(ImVec2(500, 400));
            ImGui::Begin("Select a map", nullptr, ImVec2(500, 400), 100.0f, ImGuiWindowFlags_AlwaysAutoResize);
            ImGui::Text("Select a map from the list below.");
            ImGui::Separator();
            ImGui::PushItemWidth(-1);
            ImGui::ListBox("", &mapSelectIndex, &mapFiles[0], mapFiles.size(), 10);
            ImGui::Separator();
            if (ImGui::Button("Open")) {
                //Get the name of the file
                std::vector<std::string> fullNameSplit = l2d_internal::utils::split(mapFiles[mapSelectIndex], '/');
                std::vector<std::string> fileNameSplit = l2d_internal::utils::split(fullNameSplit.back(), '.');
                this->_level.loadMap(fileNameSplit.front());
                mapSelectBoxVisible = false;
            }
            ImGui::SameLine();
            if (ImGui::Button("Cancel")) {
                mapSelectBoxVisible = false;
            }
            ImGui::End();
        }

        //New map box
        if (newMapBoxVisible) {
            ImGui::SetNextWindowPosCenter();
            ImGui::SetNextWindowSize(ImVec2(500, 400));
            static std::string newMapErrorText = "";
            ImGui::Begin("New map properties", nullptr, ImVec2(500, 400), 100.0f, ImGuiWindowFlags_AlwaysAutoResize);
            ImGui::Text("Name");
            static char name[500] = "";
            ImGui::PushItemWidth(300);
            ImGui::InputText("", name, 500);
            ImGui::PopItemWidth();
            ImGui::Separator();

            ImGui::PushItemWidth(100);

            ImGui::PushID("NewMapSize");
            ImGui::Text("Size");
            static int mapSizeX = 0;
            static int mapSizeY = 0;
            ImGui::InputInt("x", &mapSizeX, 1, 0);
            ImGui::InputInt("y", &mapSizeY, 1, 0);
            ImGui::Separator();
            ImGui::PopID();

            ImGui::PushID("NewMapTileSize");
            ImGui::Text("Tile size");
            static int mapTileSizeX = 8;
            static int mapTileSizeY = 8;
            ImGui::InputInt("x", &mapTileSizeX, 1, 0);
            ImGui::InputInt("y", &mapTileSizeY, 1, 0);
            ImGui::Separator();
            ImGui::PopID();

            ImGui::PopItemWidth();
            if (ImGui::Button("Create")) {
                if (strlen(name) <= 0) {
                    newMapErrorText = "You must enter a name for the new map!";
                }
                else if (mapSizeX < 0 || mapSizeY < 0) {
                    newMapErrorText = "You cannot have a negative map size!";
                }
                else if (mapTileSizeX < 0 || mapTileSizeY < 0) {
                    newMapErrorText = "You cannot have a negative tile size!";
                }
                else {
                    //Check if map with that name already exists. If so, give a box asking to overwrite
                    std::stringstream ss;
                    ss << l2d_internal::utils::getConfigValue("map_path") << "*";
                    std::vector<const char*> mapFiles = l2d_internal::utils::getFilesInDirectory(ss.str());
                    ss.str("");
                    ss << l2d_internal::utils::getConfigValue("map_path") << name << ".xml";
                    if (l2d_internal::utils::contains(mapFiles, ss.str())) {
                        newMapExistsOverwriteVisible = true;
                    }
                    else {
                        this->_level.createMap(std::string(name), sf::Vector2i(mapSizeX, mapSizeY),
                                               sf::Vector2i(mapTileSizeX, mapTileSizeY));
                        strcpy(name, "");
                        mapSizeX = 0;
                        mapSizeY = 0;
                        mapTileSizeX = 8;
                        mapTileSizeY = 8;
                        newMapErrorText = "";
                        newMapBoxVisible = false;
                        newMapExistsOverwriteVisible = false;
                    }
                }
            }
            ImGui::SameLine();
            if (ImGui::Button("Cancel")) {
                strcpy(name, "");
                mapSizeX = 0;
                mapSizeY = 0;
                mapTileSizeX = 8;
                mapTileSizeY = 8;
                newMapErrorText = "";
                newMapBoxVisible = false;
                newMapExistsOverwriteVisible = false;
            }
            ImGui::Text(newMapErrorText.c_str());
            if (newMapExistsOverwriteVisible) {
                newMapErrorText = "";
                ImGui::Separator();
                ImGui::Text("The name you have chosen already exists in your map directory.");
                ImGui::Text("Would you like to overwrite the existing map?");
                if (ImGui::Button("Sure!")) {
                    this->_level.createMap(std::string(name), sf::Vector2i(mapSizeX, mapSizeY),
                                           sf::Vector2i(mapTileSizeX, mapTileSizeY));
                    strcpy(name, "");
                    mapSizeX = 0;
                    mapSizeY = 0;
                    mapTileSizeX = 8;
                    mapTileSizeY = 8;
                    newMapErrorText = "";
                    newMapBoxVisible = false;
                    newMapExistsOverwriteVisible = false;
                }
                ImGui::SameLine();
                if (ImGui::Button("No!")) {
                    newMapExistsOverwriteVisible = false;
                }
            }
            ImGui::End();
        }

        if (ImGui::BeginMainMenuBar()) {
            if (ImGui::BeginMenu("File")) {
                mainHasFocus = false;
                if (ImGui::MenuItem("Exit")) {
                    this->_enabled = false; //TODO: do you want to save?
                }
                ImGui::EndMenu();
            }
            if (ImGui::BeginMenu("Edit")) {
                mainHasFocus = false;
                if (ImGui::MenuItem("Undo", "U", false, !this->_level.isUndoListEmpty())) {
                    mainHasFocus = false;
                    this->_level.undo();
                }
                if (ImGui::MenuItem("Redo", "R", false, !this->_level.isRedoListEmpty())) {
                    mainHasFocus = false;
                    this->_level.redo();
                }
                ImGui::EndMenu();
            }
            if (ImGui::BeginMenu("View")) {
                mainHasFocus = false;
                if (ImGui::Checkbox("Map Editor", &cbMapEditor)) {
                    cbAnimationEditor = false;
                    currentFeature = "Map Editor";
                }
                if (ImGui::Checkbox("Animation Editor", &cbAnimationEditor)) {
                    cbMapEditor = false;
                    currentFeature = "Animation Editor";
                }
                ImGui::EndMenu();
            }
            if (ImGui::BeginMenu("Map", cbMapEditor)) {
                mainHasFocus = false;
                if (ImGui::MenuItem("New map")) {
                    newMapBoxVisible = true;
                    mainHasFocus = false;
                }
                if (ImGui::MenuItem("Load map")) {
                    mapSelectBoxVisible = true;
                    mainHasFocus = false;
                }
                if (this->_level.getName() != "l2dSTART") {
                    if (ImGui::MenuItem("Save map")) {
                        this->_level.saveMap(this->_level.getName());
                        startStatusTimer("Map saved successfully!", 200);
                        mainHasFocus = false;
                    }
                }
                if (ImGui::MenuItem("Configure")) {
                    configWindowVisible = true;
                }
                if (this->_level.getName() != "l2dSTART") {
                    ImGui::Separator();
                    if (ImGui::BeginMenu("Add")) {
                        if (ImGui::BeginMenu("Light")) {
                            if (ImGui::MenuItem("Ambient light")) {
                                lightEditorWindowVisible = true;
                                selectedLightType = l2d_internal::LightType::Ambient;
                            }
                            if (ImGui::MenuItem("Point light")) {
                                lightEditorWindowVisible = true;
                                selectedLightType = l2d_internal::LightType::Point;
                            }
                            ImGui::EndMenu();
                        }
                        ImGui::EndMenu();
                    }
                    if (ImGui::Checkbox("Show grid lines", &cbShowGridLines)) {
                        this->_showGridLines = cbShowGridLines;
                    }
                    mainHasFocus = false;
                }
                ImGui::EndMenu();
            }
            if (ImGui::BeginMenu("Animation", cbAnimationEditor)) {
                mainHasFocus = false;
                ImGui::EndMenu();
            }
            if (ImGui::BeginMenu("Help")) {
                mainHasFocus = false;
                if (ImGui::MenuItem("About Lime2D")) {
                    aboutBoxVisible = true;
                }
                ImGui::EndMenu();
            }
            ImGui::EndMainMenuBar();
        }

        if (this->_level.getName() != "l2dSTART") {
            //Right clicking on a tile
            if (ImGui::IsMouseClicked(1)) {
                mousePos = sf::Vector2f(
                        sf::Mouse::getPosition(*this->_window).x + this->_graphics->getCamera()->getRect().left,
                        sf::Mouse::getPosition(*this->_window).y + this->_graphics->getCamera()->getRect().top);
                ImGui::OpenPopup("right click on tile");
            }
            if (ImGui::BeginPopup("right click on tile")) {
                mainHasFocus = false;
                if (ImGui::MenuItem("Properties")) {
                    tilePropertiesWindowVisible = true;
                }
                ImGui::Separator();
                ImGui::EndPopup();
            }

            //Clicking on a tile normally
            if (tileHasBeenSelected) {
                sf::Vector2f drawingMousePos(
                        sf::Mouse::getPosition(*this->_window).x + this->_graphics->getCamera()->getRect().left,
                        sf::Mouse::getPosition(*this->_window).y + this->_graphics->getCamera()->getRect().top);
                if (ImGui::IsMouseClicked(0) && mainHasFocus) {
                    sf::Vector2f tilePos(
                            (drawingMousePos.x - ((int) drawingMousePos.x % (int) (this->_level.getTileSize().x * std::stof(
                                    l2d_internal::utils::getConfigValue("tile_scale_x"))))) / this->_level.getTileSize().x /
                            (int) std::stof(l2d_internal::utils::getConfigValue("tile_scale_x")) + 1,
                            (drawingMousePos.y - ((int) drawingMousePos.y % (int) (this->_level.getTileSize().y * std::stof(
                                    l2d_internal::utils::getConfigValue("tile_scale_y"))))) / this->_level.getTileSize().y /
                            (int) std::stof(l2d_internal::utils::getConfigValue("tile_scale_y")) + 1);
                    if (tilePos.x >= 1 && tilePos.y >= 1 && tilePos.x <= this->_level.getSize().x && tilePos.y <= this->_level.getSize().y) {
                        if (this->_eraserActive) {
                            this->_level.removeTile(selectedTileLayer, sf::Vector2f((tilePos.x - 1) * this->_level.getTileSize().x * std::stof(l2d_internal::utils::getConfigValue("tile_scale_x")),
                                                                                    (tilePos.y - 1) * this->_level.getTileSize().y * std::stof(l2d_internal::utils::getConfigValue("tile_scale_y"))));
                        }
                        else {
                            this->_level.updateTile(selectedTilesetPath, selectedTilesetSize, selectedTileSrcPos,
                                                    sf::Vector2i(this->_level.getTileSize().x,
                                                                 this->_level.getTileSize().y), tilePos, 1,
                                                    selectedTileLayer);
                        }
                    }
                }
            }

            //Tile info window
            if (tilePropertiesWindowVisible) {
                ImGui::SetNextWindowPosCenter();
                ImGui::SetNextWindowSize(ImVec2(340, 400));
                ImGui::Begin("Properties", nullptr, ImVec2(340, 400), 100.0f);

                sf::Vector2f tilePos(mousePos.x - ((int) mousePos.x % (int) (this->_level.getTileSize().x * std::stof(
                        l2d_internal::utils::getConfigValue("tile_scale_x")))),
                                     (mousePos.y - ((int) mousePos.y % (int) (this->_level.getTileSize().y *
                                                                              std::stof(
                                                                                      l2d_internal::utils::getConfigValue(
                                                                                              "tile_scale_y"))))));
                ImGui::Text("Select a tile to view specific properties:");
                for(int i = 0; i < this->_level.getLayerList().size(); ++i) {
                    std::for_each(this->_level.getLayerList()[i].get()->Tiles.begin(), this->_level.getLayerList()[i].get()->Tiles.end(), [&](const std::shared_ptr<l2d_internal::Tile> &tile) {
                        if (tile.get()->getSprite().getPosition() == tilePos) {
                            if (i > 0) {
                                ImGui::SameLine();
                            }
                            if (ImGui::ImageButton(tile->getSprite(), sf::Vector2f(32.0f, 32.0f), 1, sf::Color::Transparent)) {
                                showSpecificTilePropertiesTile = std::make_shared<l2d_internal::Tile>(*tile.get());
                                showSpecificTilePropertiesLayer = std::make_shared<l2d_internal::Layer>(*this->_level.getLayerList()[i].get());
                                showSpecificTileProperties = true;
                            }
                        }
                    });
                }
                ImGui::Separator();
                ImGui::Text("Position: %d, %d", (int)tilePos.x, (int)tilePos.y);
                ImGui::Separator();
                if (showSpecificTileProperties) {
                    ImGui::Image(showSpecificTilePropertiesTile->getSprite(), sf::Vector2f(32.0f, 32.0f));
                    ImGui::Text("Layer: %d", showSpecificTilePropertiesLayer->Id);
                    if (ImGui::Button("Erase")) {
                        this->_level.removeTile(showSpecificTilePropertiesLayer->Id, tilePos);
                        showSpecificTileProperties = false;
                    }
                }
                ImGui::Separator();
                if (ImGui::Button("Close")) {
                    showSpecificTileProperties = false;
                    tilePropertiesWindowVisible = false;
                }
                ImGui::End();
            }
            //Tileset open/close event
            tilesetWindowVisible = this->_tilesetEnabled;

            //Tileset window
            if (tilesetWindowVisible) {
                static int tilesetComboIndex = -1;
                static bool showTilesetImage = false;
                static sf::Texture tilesetTexture;
                static sf::Vector2f tilesetViewSize(384, 128);
                static sf::Vector2f selectedTilePos(0,0);

                float tw = (tilesetViewSize.x * this->_level.getTileSize().x) / tilesetTexture.getSize().x;
                float th = (tilesetViewSize.y * this->_level.getTileSize().y) / tilesetTexture.getSize().y;
                static float dx = 0, dy = 0;

                ImGui::SetNextWindowPosCenter();
                ImGui::SetNextWindowSize(ImVec2(540, 300));
                ImGui::Begin("Tilesets", nullptr, ImVec2(500, 300), 100.0f, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_HorizontalScrollbar);
                std::stringstream ss;
                ss << l2d_internal::utils::getConfigValue("tileset_path") << "*";
                std::vector<const char*> tilesetFiles = l2d_internal::utils::getFilesInDirectory(ss.str());
                ImGui::PushItemWidth(400);
                if (ImGui::Combo("Select tileset", &tilesetComboIndex, &tilesetFiles[0], tilesetFiles.size())) {
                    showTilesetImage = true;
                    selectedTilesetPath = tilesetFiles[tilesetComboIndex];
                    selectedTileLayer = 1;
                    selectedTileSrcPos = sf::Vector2i(0,0);
                }
                ImGui::PopItemWidth();
                if (tilesetComboIndex > -1) {

                    ImGui::PushItemWidth(80);
                    if (ImGui::Button("+", ImVec2(20, 20))) {
                        tilesetViewSize *= 1.2f; //TODO: MAKE THIS 1.2 VALUE CONFIGURABLE
                        tw = (tilesetViewSize.x * this->_level.getTileSize().x) / tilesetTexture.getSize().x;
                        th = (tilesetViewSize.y * this->_level.getTileSize().y) / tilesetTexture.getSize().y;
                        dx = 0;
                        dy = 0;
                    }
                    ImGui::SameLine();
                    if (ImGui::Button("-", ImVec2(20, 20))) {
                        tilesetViewSize /= 1.2f;
                        tw = (tilesetViewSize.x * this->_level.getTileSize().x) / tilesetTexture.getSize().x;
                        th = (tilesetViewSize.y * this->_level.getTileSize().y) / tilesetTexture.getSize().y;
                        dx = 0;
                        dy = 0;
                    }
                    ImGui::PopItemWidth();
                    ImGui::SameLine();
                    ImGui::Text("      ");
                    ImGui::SameLine();


                    ImGui::PushItemWidth(84);
                    ImGui::PushID("nLayer");
                    ImGui::InputInt("Layer", &selectedTileLayer, 1);
                    ImGui::PopID();
                    ImGui::PopItemWidth();

                    ImGui::SameLine();
                    ImGui::Text("      ");
                    ImGui::SameLine();

                    ImGui::PushItemWidth(84);
                    ImGui::PushID("nEraser");
                    ImGui::Checkbox("Eraser", &this->_eraserActive);
                    ImGui::PopID();
                    ImGui::PopItemWidth();
                }
                if (showTilesetImage) {
                    ImGui::BeginChild("tilesetChildArea", ImVec2(500, 200), true, ImGuiWindowFlags_HorizontalScrollbar);

                    auto pos = ImGui::GetCursorScreenPos();

                    tilesetTexture = this->_graphics->loadImage(tilesetFiles[tilesetComboIndex]);
                    selectedTilesetSize = sf::Vector2i(tilesetTexture.getSize());
                    ImGui::Image(tilesetTexture, tilesetViewSize);
                    //Tileset grid
                    ImGui::SetItemAllowOverlap();

                    for (int i = 0; i < (tilesetTexture.getSize().x / this->_level.getTileSize().x) + 1; ++i) {
                        ImGui::GetWindowDrawList()->AddLine(ImVec2(pos.x + (i * tw), pos.y),
                                                            ImVec2(pos.x + (i * tw), pos.y + tilesetViewSize.y), ImColor(255,255,255,255));
                    }
                    for (int i = 0; i < (tilesetTexture.getSize().y / this->_level.getTileSize().y) + 1; ++i) {
                        ImGui::GetWindowDrawList()->AddLine(ImVec2(pos.x, pos.y + (i * th)),
                                                            ImVec2(pos.x + tilesetViewSize.x, pos.y + (i * th)), ImColor(255,255,255,255));
                    }

                    //Tileset selected item
                    //We're going to use lines for this :/
                    if (!this->_eraserActive) {
                        ImGui::GetWindowDrawList()->AddLine(
                                ImVec2(pos.x + selectedTilePos.x, pos.y + selectedTilePos.y),
                                ImVec2((pos.x + selectedTilePos.x + tw), pos.y + selectedTilePos.y),
                                ImColor(255, 0, 0, 255), 2.0f); //Top
                        ImGui::GetWindowDrawList()->AddLine(
                                ImVec2(pos.x + selectedTilePos.x, pos.y + selectedTilePos.y),
                                ImVec2(pos.x + selectedTilePos.x, (pos.y + selectedTilePos.y + th)),
                                ImColor(255, 0, 0, 255), 2.0f); //Left
                        ImGui::GetWindowDrawList()->AddLine(
                                ImVec2(pos.x + selectedTilePos.x, (pos.y + selectedTilePos.y + th)),
                                ImVec2((pos.x + selectedTilePos.x + tw), (pos.y + selectedTilePos.y + th)),
                                ImColor(255, 0, 0, 255), 2.0f); //Bottom
                        ImGui::GetWindowDrawList()->AddLine(
                                ImVec2((pos.x + selectedTilePos.x + tw), pos.y + selectedTilePos.y),
                                ImVec2((pos.x + selectedTilePos.x + tw), (pos.y + selectedTilePos.y + th)),
                                ImColor(255, 0, 0, 255), 2.0f); //Right
                    }

                    //Click event on the tileset
                    if (ImGui::IsMouseClicked(0) && ImGui::IsWindowFocused()) {
                        ImVec2 mPos = ImGui::GetMousePos();
                        dx = mPos.x - pos.x;
                        dy = mPos.y - pos.y;
                    }

                    selectedTilePos = ImVec2(tw * (static_cast<int>(dx) / static_cast<int>(tw)), th * (static_cast<int>(dy) / static_cast<int>(th)));
                    tileHasBeenSelected = true;

                    selectedTileSrcPos = sf::Vector2i((static_cast<int>(dx) / static_cast<int>(tw)) * this->_level.getTileSize().x, (static_cast<int>(dy) / static_cast<int>(th)) * this->_level.getTileSize().y);

                    ImGui::EndChild();
                }
                ImGui::End();
            }

        }

        if (lightEditorWindowVisible) {
            ImGui::SetNextWindowPosCenter();
            ImGui::SetNextWindowSize(ImVec2(300, 400));
            ImGui::Begin("Light editor", nullptr, ImVec2(300, 400), 100.0f, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_HorizontalScrollbar);
            if (selectedLightType == l2d_internal::LightType::Ambient) {
                ImGui::Text("Ambient light editor");
                ImGui::Separator();
                static ImVec4 col = ImVec4(this->_level.getAmbientColor());
                ImGui::ColorPicker3("", &col.x);
                ImVec4 col2 = {col.x, col.y, col.z, col.w};
                this->_level.setAmbientColor(col2);
                ImGui::Separator();
                static float intensity = this->_level.getAmbientIntensity();
                ImGui::SliderFloat("Intensity", &intensity, 0, 10, "%.2f");
                this->_level.setAmbientIntensity(intensity);
                ImGui::Separator();
                ImGui::Spacing();
                if (ImGui::Button("All done")) {
                    lightEditorWindowVisible = false;
                }
            }

            ImGui::End();
        }


        ImGui::Begin("Background", nullptr, ImGui::GetIO().DisplaySize, 0.0f,
                     ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove |
                     ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse |
                     ImGuiWindowFlags_NoCollapse |
                     ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoInputs |
                     ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_NoBringToFrontOnFocus);
        ImGui::GetWindowDrawList()->AddText(ImVec2(6, this->_window->getSize().y  - 20), ImColor(1.0f, 1.0f, 1.0f, 1.0f), currentFeature.c_str());
        if (showCurrentStatus) {
            ImGui::GetWindowDrawList()->AddText(ImVec2(180, this->_window->getSize().y - 20),
                                                ImColor(1.0f, 1.0f, 1.0f, 1.0f), currentStatus.c_str());
        }
        ImGui::End();

        this->_level.update(t.asSeconds());
        this->_graphics->update(t.asSeconds(), sf::Vector2f(this->_level.getTileSize()), this->_windowHasFocus);

        if (currentStatusTimer > 0) {
            currentStatusTimer -= 1;
        }
        if (currentStatusTimer <= 0) {
            showCurrentStatus = false;
        }
    }
}

void l2d::Editor::exit() {
    ImGui::SFML::Shutdown();
}
