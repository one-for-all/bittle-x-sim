from stl_to_obj.stl_to_obj import stl_to_obj

stl_files = [
    "chassis",
    "shoulder",
    "thighsym_upper",
    "kp_v5_body",
    "thighsym_lower",
    "shank",
]

for file in stl_files:
    stl_to_obj(
        "onshape/assets/" + file + ".stl",
        "www/static/mesh/" + file + ".obj",
        verbose=False,
    )
