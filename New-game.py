import pygame
import os
import re
import sys
import shutil

# Initialize pygame
pygame.init()

# Constants
SCREEN_WIDTH = 800
SCREEN_HEIGHT = 650
WHITE = (255, 255, 255)
BLACK = (0, 0, 0)
GRAY = (200, 200, 200)
LIGHT_BLUE = (173, 216, 230)
DARK_BLUE = (70, 130, 180)
GREEN = (50, 205, 50)
RED = (220, 20, 60)
ORANGE = (255, 165, 0)

# Fonts
font_large = pygame.font.SysFont('Arial', 32)
font_medium = pygame.font.SysFont('Arial', 24)
font_small = pygame.font.SysFont('Arial', 18)

class Button:
    def __init__(self, x, y, width, height, text, color=GRAY, hover_color=LIGHT_BLUE):
        self.rect = pygame.Rect(x, y, width, height)
        self.text = text
        self.color = color
        self.hover_color = hover_color
        self.is_hovered = False
        
    def draw(self, screen):
        color = self.hover_color if self.is_hovered else self.color
        pygame.draw.rect(screen, color, self.rect, border_radius=5)
        pygame.draw.rect(screen, BLACK, self.rect, 2, border_radius=5)
        
        text_surf = font_medium.render(self.text, True, BLACK)
        text_rect = text_surf.get_rect(center=self.rect.center)
        screen.blit(text_surf, text_rect)
        
    def check_hover(self, pos):
        self.is_hovered = self.rect.collidepoint(pos)
        return self.is_hovered
        
    def is_clicked(self, pos, event):
        if event.type == pygame.MOUSEBUTTONDOWN and event.button == 1:
            return self.rect.collidepoint(pos)
        return False

class Checkbox:
    def __init__(self, x, y, text, checked=False):
        self.rect = pygame.Rect(x, y, 20, 20)
        self.text = text
        self.checked = checked
        
    def draw(self, screen):
        pygame.draw.rect(screen, WHITE, self.rect)
        pygame.draw.rect(screen, BLACK, self.rect, 2)
        
        if self.checked:
            pygame.draw.rect(screen, DARK_BLUE, (self.rect.x + 4, self.rect.y + 4, 12, 12))
            
        text_surf = font_small.render(self.text, True, BLACK)
        text_rect = text_surf.get_rect(midleft=(self.rect.right + 10, self.rect.centery))
        screen.blit(text_surf, text_rect)
        
    def is_clicked(self, pos, event):
        if event.type == pygame.MOUSEBUTTONDOWN and event.button == 1:
            if self.rect.collidepoint(pos):
                self.checked = not self.checked
                return True
        return False

class GameCreator:
    def __init__(self):
        self.screen = pygame.display.set_mode((SCREEN_WIDTH, SCREEN_HEIGHT))
        pygame.display.set_caption("Game Creator - Krecek SDK")
        
        self.game_name = ""
        self.input_active = False
        self.input_rect = pygame.Rect(300, 100, 400, 40)
        
        self.checkboxes = [
            Checkbox(300, 160, "Multiplayer"),
            Checkbox(300, 190, "2D Tiles"),
            Checkbox(300, 220, "Sound"),
            Checkbox(300, 250, "Saves"),
            Checkbox(300, 280, "Music Control"),
            Checkbox(300, 310, "Scenes Mode")
        ]
        
        self.create_btn = Button(300, 380, 200, 50, "Create Game", GREEN)
        self.status_message = ""
        self.status_color = BLACK
        
        # Confirmation dialog
        self.show_confirmation = False
        self.confirm_btn = Button(250, 450, 100, 40, "Yes", GREEN)
        self.cancel_btn = Button(450, 450, 100, 40, "No", RED)
        self.confirm_message = ""
        
    def run(self):
        running = True
        clock = pygame.time.Clock()
        
        while running:
            mouse_pos = pygame.mouse.get_pos()
            
            for event in pygame.event.get():
                if event.type == pygame.QUIT:
                    running = False
                    
                if self.show_confirmation:
                    # Handle confirmation dialog
                    if self.confirm_btn.is_clicked(mouse_pos, event):
                        self.execute_game_creation()
                        self.show_confirmation = False
                    elif self.cancel_btn.is_clicked(mouse_pos, event):
                        self.show_confirmation = False
                        self.status_message = "Game creation cancelled."
                        self.status_color = ORANGE
                else:
                    # Handle text input
                    if event.type == pygame.MOUSEBUTTONDOWN:
                        self.input_active = self.input_rect.collidepoint(event.pos)
                        
                    if event.type == pygame.KEYDOWN and self.input_active:
                        if event.key == pygame.K_RETURN:
                            self.input_active = False
                        elif event.key == pygame.K_BACKSPACE:
                            self.game_name = self.game_name[:-1]
                        else:
                            self.game_name += event.unicode
                    
                    # Handle checkboxes
                    for checkbox in self.checkboxes:
                        checkbox.is_clicked(mouse_pos, event)
                    
                    # Handle create button
                    if self.create_btn.is_clicked(mouse_pos, event):
                        self.check_and_confirm()
            
            # Update button hover state
            self.create_btn.check_hover(mouse_pos)
            if self.show_confirmation:
                self.confirm_btn.check_hover(mouse_pos)
                self.cancel_btn.check_hover(mouse_pos)
            
            # Draw everything
            self.screen.fill(WHITE)
            self.draw()
            pygame.display.flip()
            clock.tick(60)
            
        pygame.quit()
    
    def draw(self):
        # Title
        title = font_large.render("Krecek SDK - Game Creator", True, DARK_BLUE)
        self.screen.blit(title, (SCREEN_WIDTH // 2 - title.get_width() // 2, 30))
        
        # Game name input
        name_label = font_medium.render("Game Name:", True, BLACK)
        self.screen.blit(name_label, (100, 105))
        
        pygame.draw.rect(self.screen, LIGHT_BLUE if self.input_active else WHITE, self.input_rect, border_radius=5)
        pygame.draw.rect(self.screen, DARK_BLUE, self.input_rect, 2, border_radius=5)
        
        text_surf = font_medium.render(self.game_name, True, BLACK)
        self.screen.blit(text_surf, (self.input_rect.x + 5, self.input_rect.y + 5))
        
        # Game type checkboxes
        type_label = font_medium.render("Game Features:", True, BLACK)
        self.screen.blit(type_label, (100, 165))
        
        for checkbox in self.checkboxes:
            checkbox.draw(self.screen)
        
        # Create button
        self.create_btn.draw(self.screen)
        
        # Status message
        if self.status_message:
            status_surf = font_small.render(self.status_message, True, self.status_color)
            self.screen.blit(status_surf, (300, 450))
        
        # Confirmation dialog
        if self.show_confirmation:
            # Semi-transparent overlay
            s = pygame.Surface((SCREEN_WIDTH, SCREEN_HEIGHT), pygame.SRCALPHA)
            s.fill((0, 0, 0, 128))  # Semi-transparent black
            self.screen.blit(s, (0, 0))
            
            # Dialog box
            dialog_rect = pygame.Rect(200, 300, 400, 200)
            pygame.draw.rect(self.screen, WHITE, dialog_rect, border_radius=10)
            pygame.draw.rect(self.screen, DARK_BLUE, dialog_rect, 2, border_radius=10)
            
            # Message
            confirm_text = font_medium.render(self.confirm_message, True, BLACK)
            self.screen.blit(confirm_text, (dialog_rect.centerx - confirm_text.get_width() // 2, dialog_rect.y + 30))
            
            # Buttons
            self.confirm_btn.draw(self.screen)
            self.cancel_btn.draw(self.screen)
    
    def check_and_confirm(self):
        if not self.game_name.strip():
            self.status_message = "Error: Game name cannot be empty!"
            self.status_color = RED
            return
        
        # Sanitize the game name
        sanitized_name = re.sub(r'[^a-zA-Z0-9_]', '_', self.game_name)
        
        # Check if game already exists
        exists_in_cmake, exists_as_dir = self.check_existing(sanitized_name)
        
        if exists_in_cmake or exists_as_dir:
            self.confirm_message = f"Game '{sanitized_name}' already exists. Overwrite?"
            self.show_confirmation = True
        else:
            # No existing game, proceed directly
            self.execute_game_creation()
    
    def execute_game_creation(self):
        sanitized_name = re.sub(r'[^a-zA-Z0-9_]', '_', self.game_name)
        
        # Remove existing directories if they exist
        src_game_dir = os.path.join("src", sanitized_name)
        host_game_dir = os.path.join("host", sanitized_name)
        
        if os.path.exists(src_game_dir):
            shutil.rmtree(src_game_dir)
        if os.path.exists(host_game_dir):
            shutil.rmtree(host_game_dir)
        
        # Create directories
        try:
            os.makedirs(src_game_dir, exist_ok=True)
            os.makedirs(host_game_dir, exist_ok=True)
        except OSError as e:
            self.status_message = f"Error creating directories: {e}"
            self.status_color = RED
            return
        
        # Update CMakeLists.txt files (will handle duplicates properly)
        self.update_cmake_files(sanitized_name)
        
        # Create template files with selected features
        self.create_template_files(sanitized_name, src_game_dir, host_game_dir, self.game_name)
        
        self.status_message = f"Successfully created game '{self.game_name}'!"
        self.status_color = GREEN
    
    def check_existing(self, game_name):
        """Check if the game already exists in CMakeLists.txt or as directories"""
        src_cmake = "src/CMakeLists.txt"
        host_cmake = "host/CMakeLists.txt"
        
        exists_in_cmake = False
        exists_as_dir = False
        
        # Check src/CMakeLists.txt
        if os.path.exists(src_cmake):
            with open(src_cmake, 'r') as f:
                content = f.read()
                if re.search(rf'add_subdirectory\s*\(\s*{re.escape(game_name)}\s*\)', content):
                    exists_in_cmake = True
        
        # Check host/CMakeLists.txt
        if os.path.exists(host_cmake):
            with open(host_cmake, 'r') as f:
                content = f.read()
                if re.search(rf'add_subdirectory\s*\(\s*{re.escape(game_name)}\s*\)', content):
                    exists_in_cmake = True
        
        # Check if directories exist
        src_dir = os.path.join("src", game_name)
        host_dir = os.path.join("host", game_name)
        
        if os.path.exists(src_dir) or os.path.exists(host_dir):
            exists_as_dir = True
        
        return exists_in_cmake, exists_as_dir
    
    def update_cmake_files(self, game_name):
        """Update CMakeLists.txt files, handling duplicates properly"""
        self.update_src_cmake(game_name)
        self.update_host_cmake(game_name)
    
    def update_src_cmake(self, game_name):
        cmake_file = "src/CMakeLists.txt"
        
        if not os.path.exists(cmake_file):
            return
        
        try:
            with open(cmake_file, 'r') as f:
                content = f.read()
            
            # Check if the game is already in the file
            pattern = rf'add_subdirectory\s*\(\s*{re.escape(game_name)}\s*\)'
            if re.search(pattern, content):
                # Already exists, no need to add again
                print(f"Game {game_name} already in {cmake_file}, skipping addition")
                return
            
            # Find the last add_subdirectory line
            pattern = r'add_subdirectory\((\w+[-]?\w*)\)'
            matches = list(re.finditer(pattern, content))
            
            if matches:
                last_match = matches[-1]
                insert_pos = last_match.end()
                
                # Insert new add_subdirectory after the last one
                new_content = (content[:insert_pos] + 
                              f"\nadd_subdirectory({game_name})" + 
                              content[insert_pos:])
            else:
                # If no add_subdirectory found, append at the end
                new_content = content + f"\nadd_subdirectory({game_name})\n"
            
            with open(cmake_file, 'w') as f:
                f.write(new_content)
            
            print(f"Updated {cmake_file}")
            
        except Exception as e:
            print(f"Error updating {cmake_file}: {e}")
    
    def update_host_cmake(self, game_name):
        cmake_file = "host/CMakeLists.txt"
        
        if not os.path.exists(cmake_file):
            return
        
        try:
            with open(cmake_file, 'r') as f:
                content = f.read()
            
            # Check if the game is already in the file
            pattern = rf'add_subdirectory\s*\(\s*{re.escape(game_name)}\s*\)'
            if re.search(pattern, content):
                # Already exists, no need to add again
                print(f"Game {game_name} already in {cmake_file}, skipping addition")
                return
            
            # Find the last add_subdirectory line
            pattern = r'add_subdirectory\((\w+[-]?\w*)\)'
            matches = list(re.finditer(pattern, content))
            
            if matches:
                last_match = matches[-1]
                insert_pos = last_match.end()
                
                # Insert new add_subdirectory after the last one
                new_content = (content[:insert_pos] + 
                              f"\nadd_subdirectory({game_name})" + 
                              content[insert_pos:])
            else:
                # If no add_subdirectory found, append at the end
                new_content = content + f"\nadd_subdirectory({game_name})\n"
            
            with open(cmake_file, 'w') as f:
                f.write(new_content)
            
            print(f"Updated {cmake_file}")
            
        except Exception as e:
            print(f"Error updating {cmake_file}: {e}")
    
    def create_template_files(self, sanitized_name, src_dir, host_dir, original_name):
        """Create template files for the new game with selected features"""
        
        scenes_mode = any(cb.checked and cb.text == "Scenes Mode" for cb in self.checkboxes)
        music_control = any(cb.checked and cb.text == "Music Control" for cb in self.checkboxes)
        
        # Create src/CMakeLists.txt
        if scenes_mode:
            src_cmake_content = f"""add_executable(
  {sanitized_name}
  main.c
  root.c
  game.c
)
target_include_directories({sanitized_name} PRIVATE include)
generate_png_headers()
target_link_libraries({sanitized_name} krecek ${{PNG_HEADERS_TARGET}})
pico_add_extra_outputs({sanitized_name})
krecek_set_target_options({sanitized_name})
"""
        else:
            src_cmake_content = f"""add_executable(
  {sanitized_name}
  main.c
)
target_include_directories({sanitized_name} PRIVATE include)
generate_png_headers()
target_link_libraries({sanitized_name} krecek ${{PNG_HEADERS_TARGET}})
pico_add_extra_outputs({sanitized_name})
krecek_set_target_options({sanitized_name})
"""
        
        # Create host/CMakeLists.txt
        if scenes_mode:
            host_cmake_content = f"""add_executable(
  {sanitized_name}
  ../../src/{sanitized_name}/main.c
  ../../src/{sanitized_name}/root.c
  ../../src/{sanitized_name}/game.c
)

generate_png_headers()
target_link_libraries({sanitized_name} PRIVATE sdk m ${{PNG_HEADERS_TARGET}})

set_property(TARGET {sanitized_name} PROPERTY C_STANDARD 23)
target_compile_options({sanitized_name} PRIVATE -Wall -Wextra -Wnull-dereference)
target_include_directories({sanitized_name} PRIVATE include)

install(TARGETS {sanitized_name} DESTINATION bin)
"""
        else:
            host_cmake_content = f"""add_executable(
  {sanitized_name}
  ../../src/{sanitized_name}/main.c
)

generate_png_headers()
target_link_libraries({sanitized_name} PRIVATE sdk m ${{PNG_HEADERS_TARGET}})

set_property(TARGET {sanitized_name} PROPERTY C_STANDARD 23)
target_compile_options({sanitized_name} PRIVATE -Wall -Wextra -Wnull-dereference)
target_include_directories({sanitized_name} PRIVATE include)

install(TARGETS {sanitized_name} DESTINATION bin)
"""
        
        # Create main.c based on selected features
        if scenes_mode:
            main_c_content = self.create_scenes_main_c(music_control, sanitized_name)
        else:
            main_c_content = self.create_standard_main_c(music_control, sanitized_name)
        
        # Write files
        try:
            # Source directory files
            with open(os.path.join(src_dir, "CMakeLists.txt"), 'w') as f:
                f.write(src_cmake_content)
            
            with open(os.path.join(src_dir, "main.c"), 'w') as f:
                f.write(main_c_content)
            
            # Create additional files for scenes mode
            if scenes_mode:
                with open(os.path.join(src_dir, "root.c"), 'w') as f:
                    f.write(self.create_root_c(music_control, sanitized_name))
                
                with open(os.path.join(src_dir, "game.c"), 'w') as f:
                    f.write(self.create_game_c(music_control, sanitized_name))
            
            # Host directory files (only CMakeLists.txt, main.c is shared from src)
            with open(os.path.join(host_dir, "CMakeLists.txt"), 'w') as f:
                f.write(host_cmake_content)
            
            print("Created template game files")
            
        except Exception as e:
            print(f"Error creating template files: {e}")
    
    def create_standard_main_c(self, music_control, sanitized_name):
        """Create standard main.c without scenes"""
        main_c_content = f"""#include <sdk.h>
"""
        
        # Add volume variable if music control is selected
        if music_control:
            main_c_content += """
float volume = 0.5f;
"""
        
        main_c_content += f"""
void game_start(void)
{{
"""
        
        # Add sdk_set_output_gain_db if music control is selected
        if music_control:
            main_c_content += """\tsdk_set_output_gain_db(volume);
"""
        
        main_c_content += f"""}}
"""

        # game_input function
        main_c_content += f"""
void game_input(unsigned dt_usec)
{{
"""
        
        if music_control:
            main_c_content += """\tfloat dt = dt_usec / 1000000.0f;

\tif (sdk_inputs.vol_up) {
\t\tvolume += 12.0 * dt;
\t}

\tif (sdk_inputs.vol_down) {
\t\tvolume -= 12.0 * dt;
\t}

\tif (sdk_inputs_delta.select > 0) {
\t\tgame_reset();
\t\treturn;
\t}

\tif (sdk_inputs_delta.vol_sw > 0) {
\t\tif (!sdk_inputs.start) {
\t\t\tif (volume < SDK_GAIN_MIN) {
\t\t\t\tvolume = 0;
\t\t\t} else {
\t\t\t\tvolume = SDK_GAIN_MIN - 1;
\t\t\t}
\t\t}
\t}

\tvolume = clamp(volume, SDK_GAIN_MIN - 1.0, 6);

\tif (sdk_inputs.vol_up || sdk_inputs.vol_down || sdk_inputs.vol_sw) {
\t\tsdk_set_output_gain_db(volume);
\t}
"""
        else:
            main_c_content += """\t(void)dt_usec; //float dt = dt_usec / 1000000.0f;
"""
        
        main_c_content += f"""}}

void game_paint(unsigned dt_usec)
{{
 (void)dt_usec;
  tft_fill(0); //reset screen every frame to 0-black                                                                                             
               }}                                                                                                
       
int main()                                                                                               
{{                                                                                                        
        struct sdk_config config = {{                                                                     
                .wait_for_usb = true,                                                                    
                .show_fps = false,                                                                       
                .off_on_select = true,                                                                   
                .fps_color = rgb_to_rgb565(31, 31, 31),                                                  
        }};                                                                                               
                                                                                                         
        sdk_main(&config);                                                                               
}}
"""
        return main_c_content
    
    def create_scenes_main_c(self, music_control, sanitized_name):
        """Create scenes-based main.c"""
        main_c_content = f"""#include <cover.png.h>
#include <sdk.h>


extern sdk_scene_t scene_root;
"""
        
        # Add volume variable if music control is selected
        if music_control:
            main_c_content += """
float volume = 0.5f;
"""
        
        main_c_content += f"""
void game_inbox(sdk_message_t msg) {{ sdk_scene_inbox(msg); }}

void game_start(void) {{ sdk_scene_push(&scene_root); }}

void game_input(unsigned dt_usec) {{
"""
        
        if music_control:
            main_c_content += """  float dt = dt_usec / 1000000.0f;

  if (sdk_inputs.vol_up) {
    volume += 12.0 * dt;
  }

  if (sdk_inputs.vol_down) {
    volume -= 12.0 * dt;
  }

  if (sdk_inputs_delta.select > 0) {
    // Handle select button for scene system
  }

  if (sdk_inputs_delta.vol_sw > 0) {
    if (!sdk_inputs.start) {
      if (volume < SDK_GAIN_MIN) {
        volume = 0;
      } else {
        volume = SDK_GAIN_MIN - 1;
      }
    }
  }

  volume = clamp(volume, SDK_GAIN_MIN - 1.0, 6);

  if (sdk_inputs.vol_up || sdk_inputs.vol_down || sdk_inputs.vol_sw) {
    sdk_set_output_gain_db(volume);
  }

"""
        
        main_c_content += f"""  sdk_scene_handle();
  sdk_scene_tick(dt_usec);
}}

void game_paint(unsigned dt_usec) {{ sdk_scene_paint(dt_usec); }}

int main() {{
  struct sdk_config config = {{
      .wait_for_usb = true,
      .show_fps = false,
      .off_on_select = true,
      .fps_color = rgb_to_rgb565(63, 63, 63),
  }};

  sdk_main(&config);
}}
"""
        return main_c_content
    
    def create_root_c(self, music_control, sanitized_name):
        """Create root.c for scenes mode"""
        root_c_content = f"""#include <sdk.h>

extern sdk_scene_t scene_game;

static void root_pushed(void) {{
    // Initialize root scene
"""

        if music_control:
            root_c_content += """    sdk_set_output_gain_db(0.5f);
"""

        root_c_content += f"""}}

static void root_popped(void) {{
    // Cleanup root scene
}}

static void root_obscured(void) {{
    // Scene is being obscured by another scene
}}

static void root_revealed(void) {{
    // Scene is being revealed after being obscured
}}

static void root_handle(sdk_event_t event, int depth) {{
    if (depth != 0) return;

    switch (event) {{
        case SDK_PRESSED_A:
            sdk_scene_push(&scene_game);
            break;
        case SDK_PRESSED_START:
            // Handle start button
            break;
        default:
            break;
    }}
}}

static void root_tick(unsigned jiffies, int depth) {{
    // Root scene tick logic
}}

sdk_scene_t scene_root = {{
    .pushed = root_pushed,
    .popped = root_popped,
    .obscured = root_obscured,
    .revealed = root_revealed,
    .handle = root_handle,
    .tick = root_tick,
}};
"""
        return root_c_content
    
    def create_game_c(self, music_control, sanitized_name):
        """Create game.c for scenes mode"""
        game_c_content = f"""#include <sdk.h>

static void game_pushed(void) {{
    // Initialize game scene
"""

        if music_control:
            game_c_content += """    sdk_set_output_gain_db(0.5f);
"""

        game_c_content += f"""}}

static void game_popped(void) {{
    // Cleanup game scene
}}

static void game_obscured(void) {{
    // Game scene is being obscured (e.g., by pause menu)
}}

static void game_revealed(void) {{
    // Game scene is being revealed after being obscured
}}

static void game_handle(sdk_event_t event, int depth) {{
    if (depth != 0) return;

    switch (event) {{
        case SDK_PRESSED_B:
            sdk_scene_pop();
            break;
        case SDK_PRESSED_START:
            // Handle pause menu or other functionality
            break;
        default:
            break;
    }}
}}

static void game_tick(unsigned jiffies, int depth) {{
    // Game logic updates
}}

static void game_paint_callback(unsigned dt_usec) {{
    // Game rendering
    tft_fill(0); // Clear screen
}}

sdk_scene_t scene_game = {{
    .pushed = game_pushed,
    .popped = game_popped,
    .obscured = game_obscured,
    .revealed = game_revealed,
    .handle = game_handle,
    .tick = game_tick,
    .paint = game_paint_callback,
}};
"""
        return game_c_content

if __name__ == "__main__":
    creator = GameCreator()
    creator.run()
