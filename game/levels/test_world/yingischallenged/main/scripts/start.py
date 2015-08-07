""" Load in saved states """

player_one.focus()
player_one.set_character_name("Benjo")
player_two.set_character_name("Bob")

coconut_one.set_weight(1)
coconut_two.set_weight(2)
coconut_three.set_weight(4)
coconut_four.set_weight(8)
coconut_five.set_weight(16)
coconut_six.set_weight(32)
coconut_seven.set_weight(64)

coconut_one.set_solidity(False);

engine.print_terminal(coconut_one.get_weight(), False)

#player_9000 = engine.create_object("characters/enemies/crocodile", "player_9000", (10, 10))

player_9000 = engine.create_object("characters/player", "player_9000", (10, 10))

engine.play_music("calm")
engine.get_objects_at((7, 2))
engine.get_objects_at((4, 4))
engine.get_objects_at((5, 4))
engine.get_objects_at((6, 4))
engine.get_objects_at((1, 1))
engine.get_objects_at((7, 4))
engine.get_objects_at((8, 4))
engine.get_objects_at((9, 4))
engine.get_objects_at(player_one.get_position())

#engine.add_button("gui/head/monkey", player_one.get_character_name(), player_one.focus)
#engine.add_button("gui/head/monkey", player_two.get_character_name(), player_two.focus)
"""
engine.add_button("gui/head/monkey", "Rock1", coconut_one.focus)
engine.add_button("gui/head/monkey", "Rock2", coconut_two.focus)
engine.add_button("gui/head/monkey", "Rock3", coconut_three.focus)
engine.add_button("gui/head/monkey", "Rock4", coconut_four.focus)
engine.add_button("gui/head/monkey", "Rock5", coconut_five.focus)
engine.add_button("gui/head/monkey", "Rock6", coconut_six.focus)
engine.add_button("gui/head/monkey", "Rock7", coconut_seven.focus)
engine.add_button("gui/head/monkey", "Portal", exit_portal.focus)
"""

croc_one.follow_path("north, east, east, east, south, south, west, west, west, north", True)
croc_two.rand_explore()
croc_three.rand_explore()

engine.print_terminal(player_one.get_position(), False)

engine.print_terminal(game.getDialogue("welcome"))

#engine.print_terminal(engine.getDialogue("welcome"))


engine.print_terminal(engine.get_tile_type((2,16)))
engine.print_terminal(engine.get_tile_type((3,16)))
engine.print_terminal(engine.get_tile_type((4,16)))
engine.print_terminal(engine.get_tile_type((5,16)))
