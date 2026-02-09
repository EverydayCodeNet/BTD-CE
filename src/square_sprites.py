"""
Square Sprites Script

Recursively finds all PNG images in the Derek_and_Harum1_Sprites_shapes directory
and converts any non-square images into square ones by centering the original sprite
on a background-colored canvas matching the larger dimension.
"""

import os
from PIL import Image

SPRITES_DIR = os.path.join(
    os.path.dirname(os.path.dirname(os.path.abspath(__file__))),
    "media", "shapes", "Derek_and_Harum1_Sprites_shapes"
)

BACKGROUND_COLOR = (88, 94, 181)


def square_sprites(directory):
    already_square = 0
    fixed = 0

    for root, _dirs, files in os.walk(directory):
        for filename in sorted(files):
            if not filename.lower().endswith(".png"):
                continue

            filepath = os.path.join(root, filename)
            img = Image.open(filepath)

            # Ensure RGB mode
            if img.mode != "RGB":
                img = img.convert("RGB")

            w, h = img.size

            if w == h:
                already_square += 1
                continue

            # Determine the larger dimension
            size = max(w, h)

            # Create new square image filled with the background color
            new_img = Image.new("RGB", (size, size), BACKGROUND_COLOR)

            # Center the original image on the new canvas
            x_offset = (size - w) // 2
            y_offset = (size - h) // 2
            new_img.paste(img, (x_offset, y_offset))

            # Overwrite the original file
            new_img.save(filepath)

            fixed += 1
            print(f"  Fixed: {filepath} ({w}x{h} -> {size}x{size})")

    return already_square, fixed


def main():
    print(f"Scanning directory: {SPRITES_DIR}")
    print()

    already_square, fixed = square_sprites(SPRITES_DIR)

    total = already_square + fixed
    print()
    print(f"Summary:")
    print(f"  Total PNG images:  {total}")
    print(f"  Already square:    {already_square}")
    print(f"  Fixed (squared):   {fixed}")


if __name__ == "__main__":
    main()
