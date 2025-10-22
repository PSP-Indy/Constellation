import sys

def parse_obj(filename):
    vertices = []
    indices = []

    with open(filename, "r") as f:
        for line in f:
            if line.startswith("v "):
                _, x, y, z = line.split()
                vertices.append((float(x), float(y), float(z)))
            elif line.startswith("f "):
                parts = line.split()[1:]
                face = [int(p.split("/")[0]) - 1 for p in parts]
                if len(face) == 3:
                    indices.extend(face)
                elif len(face) == 4:
                    indices.extend([face[0], face[1], face[2]])
                    indices.extend([face[0], face[2], face[3]])

    return vertices, indices

def write_cpp(vertices, indices, out_file="mesh_data.h"):
    with open(out_file, "w") as f:
        f.write("// Auto-generated from OBJ file\n")
        f.write("#include <vector>\n")
        f.write("#include <imgui.h>\n\n")
        
        f.write("std::vector<ImPlot3DPoint> rocket_vertices = {\n")
        for v in vertices:
            f.write(f"    {{ {v[0]:.6f}f, {v[1]:.6f}f, {v[2]:.6f}f }},\n")
        f.write("};\n\n")
        
        f.write("std::vector<unsigned int> rocket_indices = {\n")
        for i in range(0, len(indices), 3):
            f.write(f"    {indices[i]}, {indices[i+1]}, {indices[i+2]},\n")
        f.write("};\n")

if __name__ == "__main__":
    
    obj_file = "rocket.obj"
    out_file = "..\\rocket_model.hpp"
    
    verts, inds = parse_obj(obj_file)
    write_cpp(verts, inds, out_file)
    print(f"Exported {len(verts)} vertices and {len(inds)//3} triangles to {out_file}")
