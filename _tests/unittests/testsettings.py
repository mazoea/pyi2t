# coding=utf-8
import os
import logging

_this_dir = os.path.dirname(os.path.abspath(__file__))

settings = {
    "temp_dir": os.path.join(_this_dir, "temp"),
    "logging_level": logging.INFO,
}
