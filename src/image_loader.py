"""
Loads the image with all the sprites, then saves each sprite in its own RGB (no 
alpha) PNG file
"""

from pathlib import Path
from splitShapes import SplitImages as SplIm


DELETE = "__DELETE"
names = {
    "delete": {
        (0, 0): DELETE,
    },
    "misc": {
        "animations": {
            "tack_factory": {
                "silver": {
                    (2, 476): "open",
                    (2, 491): "closed",
                },
                "striped": {
                    (2, 521): "open",
                    (2, 506): "closed",
                },
                "black": {
                    (2, 536): "open",
                    (2, 551): "closed",
                },
            },
            "rangs": {
                "red_rang": {
                    (10, 437): "down",
                    (40, 588): "down",
                    (40, 601): "right",
                    (40, 614): "up",
                    (40, 627): "left",
                },
                "wood_rang": {
                    (2, 437): "down",
                    (27, 588): "down",
                    (27, 601): "right",
                    (27, 614): "up",
                    (27, 627): "left",
                },
                "yellow_glaive": {
                    (26, 720): "up",
                    (26, 734): "right",
                    (26, 744): "down",
                    (26, 762): "left",
                },
                "red_glaive": {
                    (40, 720): "up",
                    (40, 734): "right",
                    (40, 744): "down",
                    (40, 762): "left",
                },
            },
            "explosion": {
                (62, 640): "1",
                (62, 628): "2",
                (62, 617): "3",
                (62, 601): "4",
            },
            "bomber": {
                "fire": {
                    "orange": {(2, 342): "orange1"},
                    "blue": {
                        (2, 363): "blue1",
                        (2, 372): "blue2",
                        (3, 383): "blue3",
                        (3, 389): "blue4",
                        (5, 355): "blue5",
                    },
                }
            },
            "MOAB": {
                "propeller": {
                    (337, 561): "1",
                    (339, 565): "2",
                    (339, 573): "3",
                    (341, 569): "4",
                    (354, 561): "5",
                    (356, 566): "6",
                }
            },
            "monkey_arms": {
                "skin": {
                    (19, 455): "1",
                    (19, 459): "2",
                    (19, 463): "3",
                    (19, 467): "4",
                    (19, 473): "5",
                    (19, 477): "6",
                },
                "blue": {
                    (19, 481): "extended",
                    (19, 485): "short",
                },
                "red": {
                    (19, 498): "clenched",
                    (19, 507): "extended",
                },
                "grey": {
                    (19, 489): "clenched",
                    (19, 515): "extended",
                },
                "white": {
                    (19, 511): "extended",
                    (21, 533): "clenched",
                },
            },
        },
        "icons": {
            (3, 260): "pause",
            (3, 204): "bannana",
        },
        "effects": {
            "sniper": {
                (284, 201): "goggles",
                (4, 233): "muzzle_flash",
            },
            "MOAB": {
                "reinforced": {
                    (327, 550): "1",
                    (327, 530): "2",
                    (324, 540): "3",
                    (324, 520): "4",
                    (324, 479): "5",
                    (324, 499): "6",
                    (327, 489): "7",
                    (327, 509): "8",
                },
                "coatings": {
                    (137, 443): "pink",
                    (149, 443): "green",
                    (161, 443): "yellow",
                },
            },
            "regrow": {
                "coatings": {
                    (113, 416): "green",
                    (113, 453): "yellow",
                    (125, 417): "pink",
                },
            },
            "normal": {
                "coatings": {
                    (113, 437): "green",
                    (113, 474): "yellow",
                    (125, 438): "pink",
                }
            },
        },
        "chars": {
            (2, 181): "dollar",
            (10, 181): "dollar",
            (18, 181): "dollar",
            (26, 181): "dollar",
            (18, 170): "zero",
            (10, 175): "zero",
            (2, 175): "zero",
            (26, 170): "zero",
            (18, 175): "zero",
            (26, 175): "zero",
            (2, 170): "two",
            (18, 165): "two",
            (10, 169): "three",
            (26, 164): "three",
        },
        "projectiles": {
            (2, 413): "tack",
            (2, 430): "big_dart",
            "ninja_star": {
                (2, 452): "star1",
                (10, 453): "star2",
            },
            "spike_ball": {
                (12, 406): "big",
                (13, 422): "small",
            },
            "bomb": {
                (2, 462): "large",
                (17, 435): "small",
            },
        },
    },
    "bloons": {
        "MOAB": {
            (291, 491): "damaged_3",
            (225, 491): "damaged_1",
            (192, 491): "reinforced",
            (159, 491): "with_acid",
            (258, 491): "damaged_2",
            (125, 491): "un_damaged",
        },
        "regrow": {
            "non-camo": {
                "red": {
                    (126, 535): "base",
                    (188, 595): "acid",
                },
                (126, 555): "blue",
                (126, 575): "green",
                (126, 595): "yellow",
                (146, 535): "pink",
                (146, 595): "zebra",
                (147, 556): "black",
                (147, 575): "white",
                (166, 555): "rainbow",
                (166, 596): "purple",
                (186, 532): "pika",
                "ceramic": {
                    (166, 575): "ceramic1",
                    (186, 575): "reinforced",
                },
                "lead": {
                    (186, 555): "reinforced",
                    (166, 535): "base1",
                    (126, 615): "base2",
                },
            },
            "camo": {
                "ceramic": {
                    (270, 577): "reinforced",
                },
                "lead": {
                    (250, 536): "lead",
                    (228, 616): "lead2",
                    (270, 556): "reinforced",
                },
                "red": {
                    (270, 599): "acid",
                    (209, 536): "base",
                },
                (208, 576): "green",
                (228, 536): "pink",
                (270, 533): "pika",
                (248, 600): "purple",
                (228, 596): "zebra",
                (250, 577): "ceramic",
                (250, 556): "rainbow",
                (208, 556): "blue",
                (208, 596): "yellow",
                (229, 557): "black",
                (229, 576): "white",
            },
        },
        "non-regrow": {
            "non-camo": {
                "lead": {
                    (92, 452): "reinforced",
                    (70, 418): "base1",
                    (30, 486): "base2",
                },
                "ceramic": {
                    (70, 452): "non-reinforced",
                    (92, 469): "reinforced",
                },
                "red": {
                    (90, 418): "acid",
                    (30, 418): "base",
                },
                (30, 435): "blue",
                (30, 452): "green",
                (30, 469): "yellow",
                (50, 418): "pink",
                (50, 469): "zebra",
                (51, 436): "black",
                (51, 453): "white",
                (70, 435): "rainbow",
                (70, 470): "purple",
                (89, 431): "pika",
            },
            "camo": {
                "ceramic": {
                    (102, 549): "non-reinforced",
                    (82, 582): "reinforced",
                },
                "lead": {
                    (82, 547): "lead1",
                    (102, 566): "lead2",
                    (62, 582): "reinforced",
                },
                "red": {
                    (101, 496): "acid",
                    (62, 496): "base",
                },
                (83, 497): "black",
                (62, 547): "yellow",
                (62, 530): "green",
                (62, 513): "blue",
                (62, 564): "purple",
                (101, 509): "pika",
                (102, 531): "purple",
                (82, 564): "rainbow",
                (82, 530): "zebra",
                (83, 514): "white",
            },
        },
    },
    "towers": {
        "tack_pile": {
            (5, 276): "pile_of_1",
            (3, 290): "pile_of_3",
            (3, 301): "pile_of_4",
            (2, 319): "pile_of_5",
        },
        "super": {
            (108, 186): "super1",
            (132, 181): "super2",
            (60, 186): "super3",
            (83, 186): "super4",
            (37, 186): "super5",
        },
        "dart": {
            (133, 400): "dart1",
            (108, 399): "dart2",
            (55, 399): "dart3",
            (80, 399): "dart4",
            (30, 400): "dart5",
        },
        "tack_farm": {
            (123, 332): "tack1",
            (102, 332): "tack2",
            (81, 332): "tack3",
            (144, 332): "tack4",
            (60, 332): "tack5",
            (39, 332): "tack6",
        },
        "wizard": {
            (109, 380): "wizard1",
            (143, 380): "wizard2",
            (59, 380): "wizard3",
            (34, 380): "wizard4",
            (84, 379): "wizard5",
        },
        "engineer": {
            (243, 320): "engineer1",
            (209, 320): "engineer2",
            (312, 318): "engineer3",
            (139, 317): "engineer4",
            (69, 317): "engineer5",
            (34, 317): "engineer6",
            (104, 317): "engineer7",
            (277, 318): "engineer8",
            (174, 317): "engineer9",
        },
        "sniper": {
            (30, 202): "sniper1",
            (107, 202): "sniper2",
            (216, 202): "sniper3",
            (143, 202): "sniper4",
            (180, 202): "sniper5",
            (71, 202): "sniper6",
        },
        "glue": {
            (204, 266): "glue1",
            (238, 266): "glue2",
            (68, 266): "glue3",
            (35, 266): "glue4",
            (167, 267): "glue5",
            (273, 260): "glue6",
            (101, 266): "glue7",
            (134, 266): "glue8",
        },
        "catapult": {
            (182, 419): "cata1",
            (174, 458): "cata2",
            (266, 419): "cata3",
            (308, 407): "cata4",
            (224, 419): "cata5",
        },
        "mortar": {
            (153, 118): "mortar1",
            (114, 118): "mortar2",
            (191, 127): "mortar3",
            (75, 118): "mortar4",
            (36, 118): "mortar5",
            (248, 118): "mortar6",
        },
        "ice": {
            (96, 96): "ice1",
            (66, 96): "ice2",
            (36, 96): "ice3",
        },
        "bannana_farm": {
            (68, 59): "farm1",
            (37, 59): "farm2",
            (99, 59): "farm3",
        },
        "shredder": {
            (233, 34): "shredder1",
            (155, 34): "shredder2",
            (116, 34): "shredder3",
            (194, 34): "shredder4",
            (77, 34): "shredder5",
            (38, 34): "shredder6",
        },
        "gattling": {
            (329, 157): "gattling1",
            (84, 157): "gattling2",
            (133, 157): "gattling3",
            (35, 157): "gattling4",
            (182, 157): "gattling5",
            (231, 157): "gattling6",
            (280, 157): "gattling7",
        },
        "ninja": {
            (163, 302): "ninja1",
            (247, 281): "ninja2",
            (188, 281): "ninja3",
            (134, 281): "ninja4",
            (79, 281): "ninja5",
            (161, 281): "ninja6",
            (52, 281): "ninja7",
            (215, 281): "ninja8",
            (106, 281): "ninja9",
            (25, 281): "ninja10",
        },
        "bomber": {
            (20, 553): "bomber1",
            (354, 354): "bomber2",
            (30, 511): "bomber3",
            (144, 360): "bomber4",
            (375, 360): "bomber5",
            (333, 360): "bomber6",
            (207, 358): "bomber7",
            (186, 360): "bomber8",
            (30, 532): "bomber9",
            (165, 360): "bomber10",
            (249, 360): "bomber11",
            (15, 332): "bomber12",
            (15, 322): "bomber13",
            (123, 361): "bomber14",
            (41, 554): "bomber15",
            (81, 361): "bomber16",
            (10, 363): "bomber17",
            (10, 384): "bomber18",
            (22, 573): "bomber19",
            (39, 361): "bomber20",
            (41, 573): "bomber21",
            (60, 361): "bomber22",
            (102, 361): "bomber23",
            (15, 346): "bomber24",
            (228, 360): "bomber25",
            (270, 360): "bomber26",
            (291, 360): "bomber27",
            (312, 360): "bomber28",
        },
        "boomerang": {
            (216, 300): "boomerang1",
            (112, 300): "boomerang2",
            (191, 300): "boomerang3",
            (86, 300): "boomerang4",
            (60, 300): "boomerang5",
            (34, 300): "boomerang6",
            (138, 300): "boomerang7",
        },
    },
}


def flatten_naming_dict(d: dict, sfx=None):
    """
    Flatten a naming dict so that each `(x, y)` point, which identifies a shape,
    maps to a path
    """
    res = {}  # flattened dict (each entry is not a dictionary)
    for dirOrPoint, savePath in list(d.items()):
        if isinstance(dirOrPoint, tuple):
            # end target: mapping point to path
            assert isinstance(savePath, (str, Path))
            if sfx is not None:
                savePath += sfx
            res[dirOrPoint] = savePath
            continue

        # the key should be a path to a dir
        assert isinstance(dirOrPoint, (str, Path))

        # should map to a dictoinary
        assert isinstance(savePath, dict)

        subRes = flatten_naming_dict(savePath, sfx=sfx)  # maps points to paths
        for point, savePath in subRes.items():
            assert isinstance(point, tuple)
            assert isinstance(savePath, (str, Path))
            res[point] = Path(dirOrPoint).joinpath(savePath)

    return res


def main():
    """
    Opens `Derek_and_Harum1_Sprites.png`, then saves all shapes in it
    """
    path = Path("..", "media", "raw", "Derek_and_Harum1_Sprites.png")
    absolute_path = path.resolve()
    print("absolute path to sprites", absolute_path)

    splitImg = SplIm(
        path,
        backgroundInfo=SplIm.PixelColor((88, 94, 181, 255)),
        glueInfo=SplIm.PixelColor((70, 70, 70, 255)),
    )
    print("reading image")
    splitImg.readImg()

    splitImg.getShapes()
    print(f"Found {len(splitImg.shapes)} shapes")

    save_path = Path(
        "..", "media", "shapes", "Derek_and_Harum1_Sprites_shapes"
    )
    print("Saving shapes to", save_path.absolute())
    name_dict = flatten_naming_dict(names, sfx=".png")
    splitImg.saveShapes(save_path, saveDict=name_dict)
    print("Saved shapes")


if __name__ == "__main__":
    main()
