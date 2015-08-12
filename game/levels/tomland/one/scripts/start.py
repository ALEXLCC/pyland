""" Load in saved states """

player_one.focus()
player_one.set_character_name("Alex")

teleportal1.set_target(teleportal2)
teleportal2.set_target(teleportal1)

engine.play_music("beach")
engine.get_objects_at((7, 2))
engine.get_objects_at((4, 4))
engine.get_objects_at((5, 4))
engine.get_objects_at((6, 4))
engine.get_objects_at((1, 1))
engine.get_objects_at((7, 4))
engine.get_objects_at((8, 4))
engine.get_objects_at((9, 4))
engine.get_objects_at(player_one.get_position())

engine.add_dialogue(engine.get_dialogue("welcome"))
#engine.open_dialogue_box(coconut_one.focus)

croc_one.follow_path("north, east, east, north, east, south, south, south, west, west, west, north", True)
croc_two.rand_explore()
croc_three.rand_explore()
croc_four.rand_explore()
