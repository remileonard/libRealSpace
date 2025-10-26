from pprint import pprint
from PIL import Image, ImageDraw

import matplotlib.pyplot as plt
import numpy as np

def visualize_overlay_matplotlib(overlay_data, sample_rate=1):
    """
    Visualise les données overlay avec Matplotlib (interactif, zoom, etc.)
    
    Args:
        overlay_data: Dictionnaire contenant les données de l'overlay
        sample_rate: Échantillonner 1 point sur N (1 = tous les points)
    """
    vertices = overlay_data["vertices"]
    
    if not vertices:
        print("Aucun vertex trouvé")
        return
    
    # Extraire les coordonnées avec échantillonnage
    x_coords = [v["x"] for v in vertices[::sample_rate]]
    z_coords = [v["z"] for v in vertices[::sample_rate]]
    
    print(f"Affichage de {len(x_coords)} points (sur {len(vertices)} total)")
    
    # Créer le graphique
    plt.figure(figsize=(12, 10))
    plt.scatter(x_coords, z_coords, s=1, c='red', alpha=0.5)
    plt.title(f'Overlay Visualization - {len(vertices)} vertices')
    plt.xlabel('X')
    plt.ylabel('Z')
    plt.axis('equal')
    plt.grid(True, alpha=0.3)
    plt.tight_layout()
    plt.show()


overlay_strike = "c:/users/rleonard/source/repos/librealspace/out/build/windows/src/executables/debugger/Debug/dumps/MAURITANIA.TRI"
overlay_pacific = "c:/users/rleonard/source/repos/librealspace/out/build/windows/src/executables/debugger/Debug/dumps/OHAU_OVERLAY.TRI"


def read_short(byte_buffer, _offset):
    return int.from_bytes(byte_buffer[_offset:_offset + 2], byteorder='little', signed=True)

def read_int(byte_buffer, _offset):
    return int.from_bytes(byte_buffer[_offset:_offset + 4], byteorder='little', signed=True)

def read_ushort(byte_buffer, _offset):
    return int.from_bytes(byte_buffer[_offset:_offset + 2], byteorder='little', signed=False)

def read_uint(byte_buffer, _offset):
    return int.from_bytes(byte_buffer[_offset:_offset + 4], byteorder='little', signed=False)

def read_int24bits_little_endian(byte_buffer, _offset):
    b0 = byte_buffer[_offset]
    b1 = byte_buffer[_offset + 1]
    b2 = byte_buffer[_offset + 2]
    value = b0 | (b1 << 8) | (b2 << 16)
    if value & 0x800000:
        value -= 0x1000000
    return value

def draw_overlay(overlay_data, output_path="overlay_output.png", scale=1.0):
    """
    Crée une image à partir des données overlay.
    
    Args:
        overlay_data: Dictionnaire contenant les données de l'overlay
        output_path: Chemin de sortie pour l'image
        scale: Facteur d'échelle pour agrandir l'image
    """
    vertices = overlay_data["vertices"]
    
    if not vertices:
        print("Aucun vertex trouvé")
        return
    
    # Trouver les limites pour normaliser les coordonnées
    min_x = min(v["x"] for v in vertices)
    max_x = max(v["x"] for v in vertices)
    min_y = min(v["y"] for v in vertices)
    max_y = max(v["y"] for v in vertices)
    
    # Calculer les dimensions de l'image
    width = int((max_x - min_x) * scale) + 100
    height = int((max_y - min_y) * scale) + 100
    
    # Créer une image blanche
    img = Image.new('RGB', (width, height), 'white')
    draw = ImageDraw.Draw(img)
    
    # Dessiner les vertices
    for i, vertex in enumerate(vertices):
        # Normaliser et mettre à l'échelle les coordonnées
        x = int((vertex["x"] - min_x) * scale) + 50
        y = int((vertex["y"] - min_y) * scale) + 50
        
        # Dessiner un point
        radius = 3
        draw.ellipse([x - radius, y - radius, x + radius, y + radius], 
                     fill='red', outline='black')
        
        # Optionnel: afficher l'index du vertex
        # draw.text((x + 5, y), str(i), fill='blue')
    
    # Sauvegarder l'image
    img.save(output_path)
    print(f"Image sauvegardée: {output_path}")
    print(f"Dimensions: {width}x{height}")
    print(f"Nombre de vertices: {len(vertices)}")
    
    return img


def parse_overlay(file_path):
    with open(file_path, "rb") as f:
        byte_buffer = f.read()
    nb_vertices = read_short(byte_buffer, 0)
    nb_triangles = read_short(byte_buffer, 2)
    nb_quads = read_short(byte_buffer, 4)
    control = read_short(byte_buffer, 6)
    nb_triangles_plus_control = read_short(byte_buffer, 8)
    offset = 8

    if (nb_quads == 1):
        nb_triangles_plus_control = control
        control = nb_quads
        nb_quads = 0
        offset = 6
    vertices = []
    for i in range(nb_vertices):
        print(f"Lecture du vertex {i}/{nb_vertices} offset {offset}")
        u0 = byte_buffer[offset]
        offset += 1
        u1 = byte_buffer[offset]
        offset += 1
        u2 = byte_buffer[offset]
        offset += 1
        x = read_int24bits_little_endian(byte_buffer, offset)
        offset += 4
        z = read_int24bits_little_endian(byte_buffer, offset)
        offset += 4
        y = read_short(byte_buffer, offset)
        offset += 2
        vert = {
            "u": (u0, u1, u2),
            "x": x,
            "y": y,
            "z": z,
        }
        print(f"Vertex {i}: {vert}")
        vertices.append(vert)
        

    return {
        "nb_vertices": nb_vertices,
        "nb_triangles": nb_triangles,
        "nb_quads": nb_quads,
        "control": control,
        "nb_triangles_plus_control": nb_triangles_plus_control,
        "offset": offset,
        "vertices": vertices,
    }

if __name__ == "__main__":
    overlay_data = parse_overlay(overlay_strike)
    visualize_overlay_matplotlib(overlay_data)
    
    overlay_data = parse_overlay(overlay_pacific)
    visualize_overlay_matplotlib(overlay_data)
    