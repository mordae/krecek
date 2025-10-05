import os
import re
import numpy as np
from PIL import Image
import argparse

def parse_header_file(file_path):
    """Parse .h header file and extract image data"""
    with open(file_path, 'r') as f:
        content = f.read()
    
    # Extract width and height using regex
    width_match = re.search(r'#define\s+\w+_WIDTH\s+(\d+)', content)
    height_match = re.search(r'#define\s+\w+_HEIGHT\s+(\d+)', content)
    
    if not width_match or not height_match:
        print(f"Warning: Could not find WIDTH/HEIGHT in {file_path}")
        return None
    
    width = int(width_match.group(1))
    height = int(height_match.group(1))
    
    # Extract array data - look for the array declaration and content
    array_match = re.search(r'const char \w+\[\]\s*=\s*\{([^}]+)\};', content, re.DOTALL)
    if not array_match:
        print(f"Warning: Could not find array data in {file_path}")
        return None
    
    # Parse the array values
    array_str = array_match.group(1)
    # Remove comments and clean up the string
    array_str = re.sub(r'//.*', '', array_str)  # Remove inline comments
    array_str = array_str.replace('\n', ' ').replace('\t', ' ')
    
    # Extract all numbers
    numbers = []
    for match in re.finditer(r'\d+', array_str):
        numbers.append(int(match.group()))
    
    return {
        'width': width,
        'height': height,
        'data': numbers,
        'filename': os.path.basename(file_path)
    }

def create_4x4_image(image_data):
    """Create a 4x4 PNG from the image data"""
    width = image_data['width']
    height = image_data['height']
    data = image_data['data']
    
    # Create original image
    if len(data) != width * height * 3:  # RGB data
        print(f"Warning: Expected {width * height * 3} values, got {len(data)}")
        # Try to handle cases where we might have exactly width*height values (grayscale)
        if len(data) == width * height:
            # Convert grayscale to RGB
            rgb_data = []
            for val in data:
                rgb_data.extend([val, val, val])
            data = rgb_data
        else:
            return None
    
    # Create numpy array and reshape
    img_array = np.array(data, dtype=np.uint8).reshape((height, width, 3))
    
    # Create PIL image
    img = Image.fromarray(img_array, 'RGB')
    
    # Resize to 4x4 using LANCZOS resampling for good quality
    img_4x4 = img.resize((4, 4), Image.Resampling.LANCZOS)
    
    return img_4x4

def process_header_files(input_folder, output_folder):
    """Process all .h files in the input folder"""
    if not os.path.exists(output_folder):
        os.makedirs(output_folder)
    
    header_files = [f for f in os.listdir(input_folder) if f.endswith('.h')]
    
    if not header_files:
        print(f"No .h files found in {input_folder}")
        return
    
    print(f"Found {len(header_files)} .h files to process")
    
    for header_file in header_files:
        file_path = os.path.join(input_folder, header_file)
        print(f"Processing {header_file}...")
        
        try:
            # Parse the header file
            image_data = parse_header_file(file_path)
            if not image_data:
                continue
            
            # Create 4x4 image
            img_4x4 = create_4x4_image(image_data)
            if img_4x4:
                # Generate output filename
                base_name = os.path.splitext(header_file)[0]
                output_path = os.path.join(output_folder, f"{base_name}-4x4.png")
                
                # Save the image
                img_4x4.save(output_path, 'PNG')
                print(f"  Saved: {output_path}")
            else:
                print(f"  Failed to create image from {header_file}")
                
        except Exception as e:
            print(f"  Error processing {header_file}: {str(e)}")

def main():
    parser = argparse.ArgumentParser(description='Convert .h header files to 4x4 PNG images')
    parser.add_argument('--input', '-i', default='./input', 
                      help='Input folder containing .h files (default: ./input)')
    parser.add_argument('--output', '-o', default='./output',
                      help='Output folder for PNG files (default: ./output)')
    
    args = parser.parse_args()
    
    # Interactive folder selection if no arguments provided
    if args.input == './input' and args.output == './output':
        print("Header to 4x4 PNG Converter")
        print("=" * 40)
        
        input_folder = input("Enter input folder path (containing .h files): ").strip()
        output_folder = input("Enter output folder path: ").strip()
        
        if not input_folder:
            input_folder = './input'
        if not output_folder:
            output_folder = './output'
    else:
        input_folder = args.input
        output_folder = args.output
    
    # Verify input folder exists
    if not os.path.exists(input_folder):
        print(f"Error: Input folder '{input_folder}' does not exist!")
        return
    
    print(f"\nInput folder: {input_folder}")
    print(f"Output folder: {output_folder}")
    print("Starting conversion...\n")
    
    process_header_files(input_folder, output_folder)
    
    print("\nConversion completed!")

if __name__ == "__main__":
    main()
