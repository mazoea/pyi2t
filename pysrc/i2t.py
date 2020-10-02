# coding=utf-8
"""
  py wrapper for i2t
"""
import json
import os
import logging
import importlib
import sys
import ctypes
import tempfile
# =================
_logger = logging.getLogger("i2t")
_this_dir = os.path.dirname(os.path.abspath(__file__))

PY2 = (2 == sys.version_info[0])
PY3 = (3 == sys.version_info[0])

if PY2:
    class py23(object):
        basestring = basestring
else:
    class py23(object):
        basestring = str


# =================

class dir_spec(object):
    def __init__(self, base_dir=None, bin_dir=None, config_dir=None, lang_dir=None):
        base_dir = base_dir or _this_dir
        self.bins = bin_dir or os.path.join(base_dir, 'bins')
        self.configs = config_dir or os.path.join(base_dir, 'configs')
        # mind the slash
        self.lang = lang_dir or os.path.join(base_dir, 'lang_dir/')


# =================

def np_2_file(np_img):
    import cv2
    tf = tempfile.NamedTemporaryFile(mode='r', delete=False, suffix='.png')
    tf.close()
    cv2.imwrite(tf.name, np_img)
    return tf.name


def os_windows():
    """
        @return: True if the os is Windows-like.
    """
    import platform
    return u"windows" in platform.platform().lower()


def singleton(cls):
    """ Singleton decorator. """
    instances = {}

    def getinstance(*args, **kwargs):
        """ Get the instance from the underlying dict. """
        if cls not in instances:
            instances[cls] = cls(*args, **kwargs)
        return instances[cls]

    return getinstance


# =================

class _img(object):
    """
        Clean up after use.
    """

    def __init__(self, img):
        """
        :param img: file_str_or_np_img_or_pyimg
        """
        self._img = None
        self._file_str = None

        if isinstance(img, m._impl.image):
            self._img = img
            return

        if isinstance(img, py23.basestring):
            self._img = m._impl.image(img)
            return

        self._file_str = np_2_file(img)
        self._img = m._impl.image(self._file_str)

    @property
    def img(self):
        return self._img

    def __del__(self):
        if self._file_str is not None:
            try:
                if os.path.exists(self._file_str):
                    os.remove(self._file_str)
            except Exception as e:
                _logger.exception('Could not remove [%s]', self._file_str)


# =============

@singleton
class _i2t(object):
    """
        Executes image to text as an external process.
    """

    so_libs = ('libleptonica1.so.1.78.0',
               'libtesseract3-maz.so.3.0.2', 'libtesseract4-maz.so.4.1.0')

    def __init__(self):
        """ Default ctor. """
        self._impl = None
        self._deps = []
        self._dirs = None
        if os.path.exists(os.path.join(_this_dir, 'bins')):
            dirs = dir_spec(_this_dir)
            self.init(dirs)

    def init(self, dirs):
        self._dirs = dirs
        real_module_dir_str = os.path.abspath(dirs.bins)
        if real_module_dir_str not in sys.path:
            sys.path.append(real_module_dir_str)

        if os_windows():
            import pyi2t
            self._impl = pyi2t
        else:
            # load dependencies explicitly otherwise LD_LIBRARY_DIR would have to be
            # used
            for lib in self.so_libs:
                abs_lib = os.path.join(real_module_dir_str, lib)
                loaded_lib = ctypes.cdll.LoadLibrary(abs_lib)
                self._deps.append(loaded_lib)
            self._impl = importlib.import_module('pyi2t2')

        oem = self._impl.ocr_engine_manager('tesseract3', 'tesseract4')
        conf = os.path.join(dirs.configs, 'maz.v5.json')
        with open(conf, mode='r') as fin:
            js = json.load(fin)
        ocr_js = js['ocr-params']

        env3 = self._impl.env()
        for k, v in ocr_js['default'].items():
            env3[k] = str(v)

        env4 = self._impl.env()
        for k, v in ocr_js['reocr'].items():
            env4[k] = str(v)

        self.t3 = oem.ocr()
        self.t3.init(dirs.lang, 'maz', env3)
        self.t4 = oem.reocr()
        self.t4.init(dirs.lang, 'maz-lstm', env4)

    # =============

    def version(self):
        return self._impl.__version__

    # =============

    def ocr_line_v3(self, file_str_or_np_img_or_pyimg, binarize=None):
        args = _img(file_str_or_np_img_or_pyimg)
        return self._ocr_line_from_file(args.img, self.t3, binarize=binarize)

    def ocr_line_v4(self, file_str_or_np_img_or_pyimg, binarize=None):
        args = _img(file_str_or_np_img_or_pyimg)
        return self._ocr_line_from_file(args.img, self.t4, binarize=binarize)

    # =============

    def is_ib_form(self, page):
        """

        :return: True/False, str, bbox/None
        """
        ibf = self._impl.ml_ib_form(page, self._dirs.configs)
        doc_tp = ibf.type()
        is_ib = doc_tp.is_ib()
        type_str = doc_tp.str()
        ib_bbox = ibf.get_ib_section() if is_ib else None
        return is_ib, type_str, ib_bbox

    # =============

    def load_doc(self, js_str):
        """
            Load document representation from a json string.
        :param js_str:
        :return:
        """
        env = self._impl.env()
        doc = self._impl.document(env)
        doc.from_str(js_str)
        return doc

    # =============

    def create_gridline(self, page, hlines=None, vlines=None):
        """
            Create gridline object using hlines/vlines information on a page.
        :return:
        """
        hlines = page.hlines() if hlines is None else hlines
        vlines = page.vlines() if vlines is None else vlines
        return self._impl.la_gridline(hlines, vlines)

    def create_report(self, doc, cols, grid):
        """
            Create IB report object
        :param doc:
        :param pcols:
        :param pgrid:
        :param tmpl_path:
        :return:
        """
        tmpl_path = os.path.join(self._dirs.configs, 'ib-template.json')
        return self._impl.ib_report(doc, cols, grid, tmpl_path)

    def create_image_from_png(self, image_date_base64_png):
        return self._impl.image(image_date_base64_png, 'image/png')

    def create_image(self, file_str):
        return self._impl.image(file_str)

    # =============

    def detect_columns(self, doc, imgb):
        return self._impl.detect_columns(doc, imgb)

    def get_segments(self, page):
        """
            Parse `page.lines()` into segment bboxes.
        :param page: See `doc.last_page()`
        :return: List of segments
        """
        seg = self._impl.ib_page_segments_detector(page.lines())
        return seg.segments()

    # =============

    def _ocr_line_from_file(self, img, engine, binarize):
        if binarize == 'otsu':
            img.binarize_otsu()
        s, words_arr = self._impl.ocr_line(engine, img)
        return s, words_arr


m = _i2t()
