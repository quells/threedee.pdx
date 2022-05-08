import struct
import sys
from typing import Dict, List, Tuple
from vlq import vlq
import xml.etree.ElementTree as ET

def parse_float_array(s: str) -> List[float]:
    return [float(el) for el in s.split()]

class Effect():
    pass

class Material(Effect):
    def __init__(self) -> None:
        super().__init__()

    def write_to(self, f):
        raise Exception("write_to must be overloaded by subclass Material")

class Lambert(Material):
    def __init__(self, diffuse: str) -> None:
        super().__init__()
        [r, g, b, a] = parse_float_array(diffuse)
        self.luma = sum([r, g, b]) / 3.0
        self.alpha = a

    def __repr__(self) -> str:
        return "Lambert({:.2f} {:.2f})".format(self.luma, self.alpha)

    def write_to(self, f):
        """
        u8 flags u8 color
        """
        flags = 0
        color = max(0, min(127, int(self.luma * 128)))
        f.write(bytearray([flags, color]))

class Mesh():
    def __init__(self, vertices: str, normals: str, triangles: str, material_idx: int) -> None:
        self.vertices = parse_float_array(vertices)
        self.normals = parse_float_array(normals)
        self.triangles = [int(el) for el in triangles.split()]
        self.material_idx = material_idx
    
    def __repr__(self) -> str:
        return "{} {} {} {}".format(self.vertices, self.normals, self.triangles, self.material_idx)

    def write_to(self, f):
        f.write(vlq(self.material_idx)) # material

        f.write(vlq(len(self.vertices) // 3))
        for v in self.vertices:
            f.write(bytearray(struct.pack("f", v)))

        f.write(vlq(len(self.normals) // 3))
        for v in self.normals:
            f.write(bytearray(struct.pack("f", v)))

        f.write(vlq(len(self.triangles) // 9))
        for i in range(0, len(self.triangles), 9):
            f.write(vlq(i))     # v1
            f.write(vlq(i + 3)) # v2
            f.write(vlq(i + 6)) # v3
            f.write(vlq(i + 1)) # normal

def parse_collada_2005_11(root: ET.Element) -> Tuple[List[Material], List[Mesh]]:
    effs = {}
    mats = {}
    for el in root:
        if "library_effects" in el.tag:
            effs = parse_lib_eff(el)
        elif "library_materials" in el.tag:
            mats = parse_lib_mat(el, effs)
        elif "library_geometries" in el.tag:
            meshes = parse_lib_geo(el, mats)
    
    mat_list = [None] * len(mats)
    for (_, (idx, mat)) in mats.items():
        mat_list[idx] = mat
    
    return (mat_list, meshes)

def parse_lib_eff(root: ET.Element) -> Dict[str, Effect]:
    effs = {}
    for eff in root.iter("""{http://www.collada.org/2005/11/COLLADASchema}effect"""):
        id = eff.get("id")
        mat_type = None
        for el in eff.iter():
            if el.tag == """{http://www.collada.org/2005/11/COLLADASchema}lambert""":
                mat_type = "lambert"
            # TODO: handle other material types

            if el.tag == """{http://www.collada.org/2005/11/COLLADASchema}color""":
                sid = el.get("sid")
                if mat_type == "lambert" and sid == "diffuse":
                    effs[id] = Lambert(el.text)
    return effs

def parse_lib_mat(root: ET.Element, effects: Dict[str, Effect]) -> Dict[str, Tuple[int, Material]]:
    mats = {}
    idx = 0
    for mat in root.iter("""{http://www.collada.org/2005/11/COLLADASchema}material"""):
        id = mat.get("id")
        eff = mat.find("""{http://www.collada.org/2005/11/COLLADASchema}instance_effect""")
        u = eff.get("url").lstrip("#")
        mats[id] = (idx, effects[u])
        idx += 1
    return mats

def parse_lib_geo(root: ET.Element, materials: Dict[str, Tuple[int, Material]]) -> List[Mesh]:
    meshes = []
    for geo in root.iter("""{http://www.collada.org/2005/11/COLLADASchema}geometry"""):
        mesh = geo.find("""{http://www.collada.org/2005/11/COLLADASchema}mesh""")

        sources: Dict[str, List[float]] = {}
        for source in mesh.iter("""{http://www.collada.org/2005/11/COLLADASchema}source"""):
            id = source.get("id")
            arr = source.find("""{http://www.collada.org/2005/11/COLLADASchema}float_array""")
            sources[id] = arr.text

        vertices = mesh.find("""{http://www.collada.org/2005/11/COLLADASchema}vertices""")
        position = vertices.find("""{http://www.collada.org/2005/11/COLLADASchema}input[@semantic='POSITION']""")
        sources[vertices.get("id")] = sources[position.get("source").lstrip("#")]
        
        triangles = mesh.find("""{http://www.collada.org/2005/11/COLLADASchema}triangles""")
        (mat_idx, _) = materials[triangles.get("material")]
        vertices, normals = None, None
        for input in mesh.iter("""{http://www.collada.org/2005/11/COLLADASchema}input"""):
            semantic = input.get("semantic")
            source = input.get("source").lstrip("#")
            if semantic == "VERTEX":
                vertices = sources[source]
            elif semantic == "NORMAL":
                normals = sources[source]
        indices = triangles.find("""{http://www.collada.org/2005/11/COLLADASchema}p""").text
        meshes.append(Mesh(vertices, normals, indices, mat_idx))
    return meshes

def write_to(f, materials: List[Material], meshes: List[Mesh]):
    f.write(bytearray("KW3D", "ASCII"))
    f.write(bytearray([0, 1])) # v0.1

    f.write(vlq(len(materials)))
    for mat in materials:
        mat.write_to(f)
    
    f.write(vlq(len(meshes)))
    for mesh in meshes:
        mesh.write_to(f)

def main():
    filename = sys.argv[1] # TODO: check input
    with open(filename) as f:
        tree = ET.parse(filename)
    root = tree.getroot()

    if root.tag != """{http://www.collada.org/2005/11/COLLADASchema}COLLADA""":
        print("unsupported version {}", root.tag)
        sys.exit(1)

    (materials, meshes) = parse_collada_2005_11(root)
    with open("out.3d", "w+b") as f:
        write_to(f, materials, meshes)

if __name__ == "__main__":
    main()
