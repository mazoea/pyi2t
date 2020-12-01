# coding=utf-8
import os
import sys
import glob

_this_dir = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, os.path.join(_this_dir, "../../pysrc"))
main_dir = os.path.join(_this_dir, '../../../../')
config_dir = os.path.join(main_dir, 'bits/lang_dir/configs/')
lang_dir = os.path.join(main_dir, 'bits/lang_dir/')
test_data_dir = os.path.join(main_dir, '_tests/data')


g_i2t = None


def get_i2t(dirs=None):
    global g_i2t
    if g_i2t is not None:
        return g_i2t
    from i2t import m
    dirs = dirs or get_dirs()
    g_i2t = m
    g_i2t.init(dirs)
    return g_i2t


def os_windows():
    """
        @return: True if the os is Windows-like.
    """
    import platform
    return u"windows" in platform.platform().lower()


def get_dirs():
    bin_dir = None
    if os_windows():
        for d in ('../../../../bin/Debug/', ):
            arr = glob.glob(os.path.join(_this_dir, d, 'pyi2t*pyd'))
            if len(arr) == 1:
                bin_dir = os.path.join(_this_dir, d)
                break
    else:
        for d in ('../../../../bin-nix/', ):
            arr = glob.glob(os.path.join(_this_dir, d, 'pyi2t*so'))
            if len(arr) == 1:
                bin_dir = os.path.join(_this_dir, d)
                break
    from i2t import dir_spec
    return dir_spec(bin_dir=bin_dir, config_dir=config_dir, lang_dir=lang_dir)
