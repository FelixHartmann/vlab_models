class CellComplex3D(Definition):
    template_name = "cellflips::CellComplex"
    default_name = "CellComplex3D"
    arguments_order = ("vertex", "edge", "face", "cell")
    typedefs = {
                 "vertex": ("vertex_t",""),
                 "edge": ("edge_t",""),
                 "face": ("face_t",""),
                 "cell": ("cell_t",""),
                 "oriented_vertex": ("oriented_vertex_t",""),
                 "oriented_edge": ("oriented_edge_t",""),
                 "oriented_face": ("oriented_face_t",""),
                 "oriented_cell": ("oriented_cell_t",""),
               }

