import pygame
import os
import re
import sys

# Initialize Pygame
pygame.init()

# Constants
SCREEN_WIDTH = 800
SCREEN_HEIGHT = 600
BG_COLOR = (40, 44, 52)
TEXT_COLOR = (220, 220, 220)
HIGHLIGHT_COLOR = (86, 182, 194)
BUTTON_COLOR = (61, 90, 128)
BUTTON_HOVER_COLOR = (78, 110, 154)
SAVE_BUTTON_COLOR = (152, 195, 121) # Greenish color for save
MENU_BG_COLOR = (30, 33, 40)

class TextureViewer:
    def __init__(self):
        self.screen = pygame.display.set_mode((SCREEN_WIDTH, SCREEN_HEIGHT))
        pygame.display.set_caption("Texture Viewer & Exporter")
        self.clock = pygame.time.Clock()
        self.font = pygame.font.SysFont('Arial', 16)
        self.title_font = pygame.font.SysFont('Arial', 24, bold=True)
        
        self.textures = {}
        self.current_texture = None
        self.current_texture_name = None
        self.menu_open = True
        self.texture_list = []
        self.load_textures_from_directory()
        
        self.scroll_offset = 0
        self.status_message = ""
        self.status_timer = 0

    def load_textures_from_directory(self):
        """Load textures from .h files in the current directory"""
        for filename in os.listdir('.'):
            if filename.endswith('.h'):
                self.parse_texture_file(filename)

    def parse_texture_file(self, filename):
        """Parse a .h file to extract texture data"""
        try:
            with open(filename, 'r') as file:
                content = file.read()
            
            name_match = re.search(r'const char (\w+)\[\]', content)
            if not name_match: return
                
            texture_name = name_match.group(1)
            width_match = re.search(r'#define\s+' + texture_name + '_WIDTH\s+(\d+)', content)
            height_match = re.search(r'#define\s+' + texture_name + '_HEIGHT\s+(\d+)', content)
            
            if not width_match or not height_match: return
                
            width = int(width_match.group(1))
            height = int(height_match.group(1))
            
            array_match = re.search(r'=\s*\{([^}]+)\}', content, re.DOTALL)
            if not array_match: return
                
            array_data = re.sub(r'\s+', ' ', array_match.group(1)).strip()
            numbers = [int(x.strip()) for x in array_data.split(',') if x.strip()]
            
            surface = pygame.Surface((width, height))
            pixel_array = pygame.PixelArray(surface)
            
            idx = 0
            for y in range(height):
                for x in range(width):
                    if idx + 2 < len(numbers):
                        pixel_array[x, y] = (numbers[idx], numbers[idx+1], numbers[idx+2])
                        idx += 3
            
            pixel_array.close()
            self.textures[texture_name] = {'surface': surface, 'width': width, 'height': height}
            self.texture_list.append(texture_name)
        except Exception as e:
            print(f"Error parsing {filename}: {e}")

    def save_current_as_png(self):
        """Exports the current surface to a PNG file"""
        if self.current_texture:
            filename = f"{self.current_texture_name}.png"
            try:
                pygame.image.save(self.current_texture, filename)
                self.status_message = f"Saved: {filename}"
                self.status_timer = 120 # Show for 2 seconds (60fps)
            except Exception as e:
                self.status_message = "Error saving file!"
                print(e)

    def draw_menu(self):
        self.screen.fill(BG_COLOR)
        title = self.title_font.render("Texture Viewer", True, TEXT_COLOR)
        self.screen.blit(title, (SCREEN_WIDTH // 2 - title.get_width() // 2, 20))
        
        menu_rect = pygame.Rect(SCREEN_WIDTH // 4, 120, SCREEN_WIDTH // 2, SCREEN_HEIGHT - 200)
        pygame.draw.rect(self.screen, MENU_BG_COLOR, menu_rect, border_radius=10)
        
        item_height = 40
        visible_items = (menu_rect.height - 20) // item_height
        start_index = max(0, min(self.scroll_offset, len(self.texture_list) - visible_items))
        
        for i in range(start_index, min(start_index + visible_items, len(self.texture_list))):
            texture_name = self.texture_list[i]
            item_rect = pygame.Rect(menu_rect.x + 10, menu_rect.y + 10 + (i - start_index) * item_height, menu_rect.width - 20, item_height - 5)
            color = HIGHLIGHT_COLOR if texture_name == self.current_texture_name else BUTTON_COLOR
            pygame.draw.rect(self.screen, color, item_rect, border_radius=5)
            
            name_text = self.font.render(texture_name, True, TEXT_COLOR)
            self.screen.blit(name_text, (item_rect.x + 10, item_rect.y + item_rect.height // 2 - name_text.get_height() // 2))

    def draw_texture_view(self):
        self.screen.fill(BG_COLOR)
        if self.current_texture:
            # Info Text
            name_text = self.title_font.render(self.current_texture_name, True, TEXT_COLOR)
            self.screen.blit(name_text, (20, 20))
            
            # Display Texture (Scaled)
            scale = min((SCREEN_WIDTH - 100) / self.textures[self.current_texture_name]['width'], (SCREEN_HEIGHT - 150) / self.textures[self.current_texture_name]['height'])
            sw, sh = int(self.textures[self.current_texture_name]['width'] * scale), int(self.textures[self.current_texture_name]['height'] * scale)
            scaled_tex = pygame.transform.scale(self.current_texture, (sw, sh))
            self.screen.blit(scaled_tex, ((SCREEN_WIDTH - sw)//2, (SCREEN_HEIGHT - sh)//2 + 20))

            # UI Buttons
            self.btn_menu = pygame.Rect(SCREEN_WIDTH - 120, 20, 100, 35)
            self.btn_save = pygame.Rect(SCREEN_WIDTH - 120, 65, 100, 35)
            
            pygame.draw.rect(self.screen, BUTTON_COLOR, self.btn_menu, border_radius=5)
            pygame.draw.rect(self.screen, SAVE_BUTTON_COLOR, self.btn_save, border_radius=5)
            
            self.screen.blit(self.font.render("Menu", True, TEXT_COLOR), (self.btn_menu.centerx - 20, self.btn_menu.centery - 8))
            self.screen.blit(self.font.render("Save PNG", True, BG_COLOR), (self.btn_save.centerx - 35, self.btn_save.centery - 8))

            # Status Message
            if self.status_timer > 0:
                msg = self.font.render(self.status_message, True, SAVE_BUTTON_COLOR)
                self.screen.blit(msg, (20, SCREEN_HEIGHT - 40))
                self.status_timer -= 1

    def handle_events(self):
        for event in pygame.event.get():
            if event.type == pygame.QUIT: return False
            if event.type == pygame.MOUSEBUTTONDOWN:
                pos = pygame.mouse.get_pos()
                if self.menu_open:
                    # Simplified selection logic for the demo
                    menu_rect = pygame.Rect(SCREEN_WIDTH // 4, 120, SCREEN_WIDTH // 2, SCREEN_HEIGHT - 200)
                    if menu_rect.collidepoint(pos):
                        idx = (pos[1] - 130) // 40 + self.scroll_offset
                        if idx < len(self.texture_list):
                            self.current_texture_name = self.texture_list[idx]
                            self.current_texture = self.textures[self.current_texture_name]['surface']
                            self.menu_open = False
                else:
                    if self.btn_menu.collidepoint(pos): self.menu_open = True
                    if self.btn_save.collidepoint(pos): self.save_current_as_png()
        return True

    def run(self):
        while self.handle_events():
            if self.menu_open: self.draw_menu()
            else: self.draw_texture_view()
            pygame.display.flip()
            self.clock.tick(60)
        pygame.quit()

if __name__ == "__main__":
    # Ensure T_00.h exists for testing if needed...
    app = TextureViewer()
    app.run()
