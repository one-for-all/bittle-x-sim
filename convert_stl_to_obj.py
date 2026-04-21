from stl_to_obj.stl_to_obj import stl_to_obj

stl_files = [
    "chassis",
    "shoulder",
    "thighsym_upper",
    "kp_v5_body",
    "thighsym_lower",
    "shank",
    "cover",
    "front",
    "neck",
    "head_1",
    "head_2",
    "head_3",
    "tail",
    "battlid",
    "battcase",
    "809_25_v1",
    "springs_v2",
]

for file in stl_files:
    stl_to_obj(
        "onshape/assets/" + file + ".stl",
        "www/static/mesh/" + file + ".obj",
        verbose=False,
        precision=6,
    )
