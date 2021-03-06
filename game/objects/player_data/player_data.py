import operator
import os

import sys
sys.path.insert(1, os.path.dirname(os.path.realpath(__file__)) + '/..')
from game_object import GameObject

import json

#TODO: add checks in all of these functions so that states can be lazily made etc.

""" This class is a useful object which is useful in that automatically handels a lot of the
saving that the Pyland game requires.


All the save stuff from engine needs to be abstracted out to here as ultimately this will off a more robust and flexible system

"""
class PlayerData(GameObject):


    __current_world = None
    __current_level = None
    __current_map = None


    __player_name = None

    __player_data = None
    __level_data = None

    def initialise(self):
        pass

    def create(self, player_name):
        """ create a new save-game with the name given """
        engine = self.get_engine()
        save_string = ""
        with open(engine.get_config()['files']['initial_save_location'], encoding="utf8") as initial_save_file:
            save_string = initial_save_file.read()
        json_data = json.loads(save_string)
        engine.save_player_data(player_name, json_data)
        return

    def __get_player_data_value(self, data_name, default = 0):
        if not (data_name in self.__player_data):
            return default
        else:
            return self.__player_data[data_name]

    def __set_player_data_value(self, data_name, value):
        self.__player_data[data_name] = value
        return

    def __get_number_pyscripter_tabs(self):
        return 1

    def __get_number_totems(self):
        return 1

    def __has_unlocked_pyscripter(self):
        return self.__get_player_data_value("has_unlocked_pyscripter", False)

    def unlock_pyscripter(self):
        self.__set_player_data_value("has_unlocked_pyscripter", True)

    def __get_pyscipter_speed(self):
        return 3.5

    def is_level_completed(self, full_level_name):
        completed_levels = self.__get_completed_levels()
        if not full_level_name in completed_levels:
            return False
        else:
            return completed_levels[full_level_name]

    def complete_level_and_save(self):
        self.__get_completed_levels()["/" + self.__current_world + "/" + self.__current_level] = True
        self.save()


    def __get_completed_levels(self):
        if not ("completed_levels" in self.__player_data):
            self.__player_data["completed_levels"] = {}
        return self.__player_data["completed_levels"]

    def load(self, player_name):
        """ loads in the player data, and correctly set's up all the global states of the game.

        For example, it displays the correct number of totems, PyScipter tabs etc.
        """
        #TODO: Make handling a save not existing a lot nicer
        engine = self.get_engine()
        self.__player_name = player_name
        self.__player_data = engine.get_player_data(player_name)
        if not self.__player_data:
            self.create(player_name)
            self.__player_data = engine.get_player_data(player_name)

        #setup all the player-data (what they have unlocked)

        #set if the pyscripter is unlocked
        if(self.__has_unlocked_pyscripter()):
            engine.show_py_scripter()
        else:
            engine.hide_py_scripter()

        #load in any scripts that might have been saved from the last time the player played
        if "py_scripter_text" in self.__player_data:
            for tab_number in self.__player_data["py_scripter_text"]:
                if self.__player_data["py_scripter_text"][tab_number] != engine.get_script(tab_number = tab_number):
                    engine.clear_scripter(tab_number = tab_number)
                    engine.insert_to_scripter(self.__player_data["py_scripter_text"][tab_number], tab_number = tab_number)

        #show the correct level and world TODO: Make it extract the correct level, worlds, and unlocked totems for the world!
        engine.update_world_text("1")
        engine.update_level_text("2")
        engine.update_totems_text(0, 5)

        #change the player sprite to be what is saved in the player_data
        player_one = engine.get_object_called("player_one")
        player_one.change_state(self.__get_player_data_value("player_type", default = "female_0"))
        return

    def save(self):
        engine = self.get_engine()
        self.__player_data["py_scripter_text"] = {}
        unlocked_tabs = engine.get_py_tabs()
        for tab_number in range(0, unlocked_tabs):
            self.__player_data["py_scripter_text"][tab_number] = engine.get_script(tab_number = tab_number)
        engine.save_player_data(self.__player_name, self.__player_data)
        return

    def set_map(self, world_name, level_name = None, map_name = None):
        """ set the current map of the game. This is done when every map is loading so that
        the latest player map-state will always be saved """
        self.__current_world = world_name
        self.__current_level = level_name
        self.__current_map = map_name

        self.__player_data["current_map"] = self.get_full_map_name()
        self.save()
        return

    def get_full_map_name(self):
        full_map_name = "/" + self.__current_world
        if self.__current_level != None:
            full_map_name += "/" + self.__current_level
        if self.__current_map != None:
            full_map_name += "/" + self.__current_map
        return full_map_name

    def previous_exit_is(self, world_name, level_name = None, map_name = None, info = None):
        """ Check to see where the player perviously exited the game. Can check to see how much matches """
        check_string = "/" + world_name
        if(level_name != None):
            check_string += "/" + level_name
        if(map_name != None):
            check_string += "/" + map_name
        if(info != None):
            check_string += "/" + info
        return (check_string in self.__player_data["previous_exit"])

    def save_and_exit(self, destination, info = None):
        """ Go to the given destination and save the game """
        previous_exit = self.get_full_map_name()
        if(info != None):
            previous_exit += "/" + info
        self.__player_data["previous_exit"] = previous_exit
        self.save()
        self.get_engine().change_map(destination)

    def save_checkpoint(self, checkpoint_name, callback = lambda: None):
        previous_exit = self.get_full_map_name() + "/" + checkpoint_name
        self.__player_data["previous_exit"] = previous_exit
        self.save()
        self.get_engine().add_event(callback)

    def __get_level_data(self):
        if not (self.__current_world in self.__player_data["level_data"]):
            self.__player_data["level_data"][self.__current_world] = {}
        if not (self.__current_level in self.__player_data["level_data"][self.__current_world]):
            self.__player_data["level_data"][self.__current_world][self.__current_level] = {}
        return self.__player_data["level_data"][self.__current_world][self.__current_level]

    def get_level_state(self, level_state):
        """ Get some kind of state associated with the level, the default value is 0, otherwise it returns the number saved there. """
        level_data = self.__get_level_data()
        if not (level_state in level_data):
            return 0
        else:
            return level_data[level_state]

    def set_level_state(self, level_state, value):
        #TODO: add checks to this so that if the state doesn't exist it is created
        self.__get_level_data()[level_state] = value

    def __get_totems(self):
        if not "totems" in self.__player_data:
            self.__player_data["totems"] = {}
        return self.__player_data["totems"]
    
    def unlock_totem(self, totem_number, callback = lambda: None):
        """ Unlock the totem with the given id """
        unlocked_totems = self.__get_totems()
        unlocked_totems[totem_number] = True
        callback()


