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
import time
import base64

# =================
_logger = logging.getLogger("i2t")
_this_dir = os.path.dirname(os.path.abspath(__file__))

PY2 = (2 == sys.version_info[0])

if PY2:
    class py23(object):
        basestring = basestring
else:
    class py23(object):
        basestring = str


class perf_probe(object):
    """
        Performance probe logging time it took.
    """

    ENABLED = False

    def __init__(self, msg, min_t=0.01, enabled=ENABLED, raw_print=False):
        self._enabled = enabled
        if self.disabled:
            return
        self.msg = msg
        self.s = None
        self.min_t = min_t
        self.raw_print = raw_print

    @property
    def disabled(self):
        return not self._enabled

    def __enter__(self):
        if self.disabled:
            return
        self.s = time.time()
        return self

    def __exit__(self, *args):
        if self.disabled:
            return
        took = time.time() - self.s
        if took < self.min_t:
            return
        msg = '[%10s] took [%8.4fs]' % (self.msg, took)
        if self.raw_print:
            print(msg)
        else:
            _logger.debug(msg)


def perf_method(min_t=0.2):
    def wrap(func):
        def _enclose(self, *args, **kw):
            with perf_probe(func.__name__, min_t):
                return func(self, *args, **kw)

        return _enclose

    return wrap


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


def np_to_png_base64(np_img):
    import cv2
    img_str = cv2.imencode('.png', np_img)[1].tostring()
    return base64.standard_b64encode(img_str).decode("ascii")


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

        # faster than `np_to_png_base64`
        self._file_str = np_2_file(img)
        self._img = m._impl.image(self._file_str)

        # slower
        # base64_buf = np_to_png_base64(img)
        # self._img = m._impl.image(base64_buf, 'image/png')

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
        self._path = None
        self._deps = []
        self._dirs = None
        if os.path.exists(os.path.join(_this_dir, 'bins')):
            dirs = dir_spec(_this_dir)
            self.init(dirs)
        self.t3 = None
        self.t4 = None

    def init(self, dirs):
        _logger.debug('OCR models loading')
        self._dirs = dirs
        real_module_dir_str = os.path.abspath(dirs.bins)
        if real_module_dir_str not in sys.path:
            sys.path.append(real_module_dir_str)

        import locale
        locale.setlocale(locale.LC_ALL, 'C')

        if os_windows():
            import pyi2t
            self._impl = pyi2t
        else:
            # load dependencies explicitly otherwise LD_LIBRARY_DIR would have to be
            # used
            # IF you encounter with PyCHARM an `invalid free()` or similar, disable
            # `File > Settings > Tools >  Python > show plots in tool window`
            for lib in self.so_libs:
                abs_lib = os.path.join(real_module_dir_str, lib)
                loaded_lib = ctypes.cdll.LoadLibrary(abs_lib)
                self._deps.append(loaded_lib)
            if PY2:
                self._impl = importlib.import_module('pyi2t2')
            else:
                self._impl = importlib.import_module('pyi2t3')
        self._path = self._impl.__file__
        if os.environ.get('MAZ_EXT_OCR_MODELS', '1') == '0':
            _logger.debug('OCR models (lazy) loaded')
            return

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
        _logger.info('OCR models loaded')

    def bin_path(self) -> str:
        """
            Return binary path
        """
        return self._path

    # =============

    def version(self):
        return self._impl.__version__

    # =============

    @perf_method()
    def image_wrapper(self, file_str_or_np_img_or_pyimg):
        return _img(file_str_or_np_img_or_pyimg)

    @perf_method()
    def ocr_line_v3(self, file_str_or_np_img_or_pyimg, binarize=None):
        args = self.image_wrapper(file_str_or_np_img_or_pyimg)
        return self._ocr_line(args.img, self.t3, binarize=binarize)

    def ocr_line_v4(self, file_str_or_np_img_or_pyimg, binarize=None):
        args = self.image_wrapper(file_str_or_np_img_or_pyimg)
        return self._ocr_line(args.img, self.t4, binarize=binarize)

    def reocr_v4(self, i2t_page_img, i2t_bbox, raw=False):
        return self._reocr(i2t_page_img, i2t_bbox, self.t4, raw)

    def ocr_block_v3(self, file_str_or_np_img_or_pyimg, binarize=None):
        args = self.image_wrapper(file_str_or_np_img_or_pyimg)
        return self._ocr_block(args.img, self.t3, binarize=binarize)

    def ocr_block_v4(self, file_str_or_np_img_or_pyimg, binarize=None):
        args = self.image_wrapper(file_str_or_np_img_or_pyimg)
        return self._ocr_block(args.img, self.t4, binarize=binarize)

    def ocr_word_v3(self, file_str_or_np_img_or_pyimg, binarize=None):
        args = self.image_wrapper(file_str_or_np_img_or_pyimg)
        return self._ocr_word(args.img, self.t3, binarize=binarize)

    def ocr_word_v4(self, file_str_or_np_img_or_pyimg, binarize=None):
        args = self.image_wrapper(file_str_or_np_img_or_pyimg)
        return self._ocr_word(args.img, self.t4, binarize=binarize)


    # =============

    def is_ib_form(self, page, trans_bbox):
        """
            :return: True/False, str, bbox/None

            In i2t, `ml_ib_form` (`ml::classify::ib_form`) is called from `classify_form` in
            `::ocr` and `::reocr` stages. The classifier is serialized into `page.info["ib:classify"]`
            and contains:
                * type - str (no, report, invalid)
                * features - dict
                * ib_section - bbox
        """
        ibf = self._impl.ml_ib_form(self._dirs.configs)
        ibf.process(page)
        doc_tp = ibf.type()
        is_ib = doc_tp.is_ib()
        ib_bbox = ibf.get_ib_section() if is_ib else None
        features = ibf.json_features_s()
        try:
            features = json.loads(features)
        except Exception:
            pass
        type_s = doc_tp.str()  # no, report, invalid
        if type_s == "report":
            type_s = "IB"

        d = {
            "valid": True,
            "type": type_s,
            "ib_section": trans_bbox(ib_bbox),
            "features": features,
        }
        return is_ib, d

    def ml_ib_form_prepare(self, doc):
        self._impl.ml_ib_form_prepare(doc)

    def is_ib_in_ub_form(self, page, img, trans_bbox):
        # TODO TM hack, that image need to process, if it is not from i2t
        process_img = True if isinstance(img, str) else False
        pyi2t_img = self.image_wrapper(img)
        ub_templ = os.path.join(self._dirs.configs, "ub04-bbox-template.json")
        ubf = self._impl.ub04_form.classify(pyi2t_img.img, ub_templ, process_img)
        d = {
            "valid": ubf.valid(),
            "type": "no",
            "ib_section": None,
            # "valid_perc": ubf.valid_perc(),
            #"ub_bbox": ubf.bbox(),
            "dbg": ubf.dbg_info_str()
        }
        if not ubf.valid():
            return False, d

        is_ib = self._impl.classify_ib_in_ub(page, ubf)
        d["type"] = "IB" if is_ib else "no"
        d["customer"]: ubf.customer()
        d["bill_type"]: ubf.bill_type()
        d["ib_section"] = trans_bbox(ubf.line_section())
        d["dbg"] = ubf.dbg_info_str()

        return is_ib, d

    # =============

    def load_doc(self, js_str):
        """
            Load document representation from a json string.
        """
        env = self._impl.env()
        doc = self._impl.document(env)
        doc.from_str(js_str)
        return doc

    # =============

    def create_grid_info(self, page, hlines=None, vlines=None):
        """
            Create gridline object using hlines/vlines information on a page.
        """
        hlines = page.hlines() if hlines is None else hlines
        vlines = page.vlines() if vlines is None else vlines
        return self._impl.la_gridline(hlines, vlines)

    def create_grid_rows(self, gridcells):
        """
            Create grid rows using GridFinder
        """
        return self._impl.la_gridrows(gridcells)

    def create_report(self, doc, cols, grid, imgb, dbg=''):
        """
            Create IB report object
        """
        tmpl_path = os.path.join(self._dirs.configs, 'ib-template.json')
        return self._impl.create_report(doc, cols, grid, tmpl_path, imgb, dbg=dbg)

    def create_image_from_png(self, image_date_base64_png):
        return self._impl.image(image_date_base64_png, 'image/png')

    def create_image(self, file_str):
        return self._impl.image(file_str)

    def create_bbox(self, xlt, ylt, xrb, yrb):
        return self._impl.bbox_type(xlt, ylt, xrb, yrb)

    # =============

    def ia_lines(self, imgb, letter_h, dbg=''):
        return self._impl.ia_lines(imgb, letter_h, dbg)

    # =============

    def extend_grid(self):
        return self._impl.extend_grid()

    def load_columns_from_grid(self, doc, grid_info, grid):
        return self._impl.load_columns_from_grid(doc, grid_info, grid)

    def create_columns(self, page):
        """
            Create empty columns with minimal columns
            set but without any rows.
        """
        return self._impl.create_columns(page)

    def columns_from_table(self, page, imgb, ib_bbox, dbg=''):
        """
            Try to use the columns detected by the (specific) table layout.
        """
        return self._impl.columns_from_table(page, imgb, ib_bbox, dbg)

    def detect_columns(self, page, imgb, grid_info):
        """
            Detect columns on a page.
        """
        return self._impl.detect_columns(page, imgb, grid_info)

    def rough_ib_lines(self, lines):
        return self._impl.rough_ib_lines(lines)

    def get_simple_segments(self, page, hlines):
        """
            Parse `page.lines()` into segment bboxes.
        :param page: See `doc.last_page()`
        :return: List of segments
        """
        hlines = [] if hlines is None else hlines
        seg = self._impl.ib_page_segments_detector(page.lines(), hlines)
        return seg.segments()

    def subtypes_dict(self, subtypes_arr):
        s = self._impl.json_subtypes_s(subtypes_arr)
        return json.loads(s)

    # =============

    def _ocr_line(self, img, engine, binarize):
        with perf_probe('binarize'):
            if binarize == 'otsu':
                img.binarize_otsu()
        with perf_probe('ocr_line'):
            s, words_arr = self._impl.ocr_line(engine, img, False)
        return s, words_arr

    def _reocr(self, i2t_page_img, i2t_bbox, engine, raw=False):
        with perf_probe('reocr'):
            s, words_arr = self._impl.reocr(engine, i2t_page_img, i2t_bbox, raw)
        return s, words_arr

    def _ocr_block(self, img, engine, binarize):
        with perf_probe('binarize'):
            if binarize == 'otsu':
                img.binarize_otsu()
        with perf_probe('ocr_line'):
            s, words_arr = self._impl.ocr_block(engine, img)
        return s, words_arr

    def _ocr_word(self, img, engine, binarize):
        with perf_probe('binarize'):
            if binarize == 'otsu':
                img.binarize_otsu()
        with perf_probe('ocr_line'):
            s, words_arr = self._impl.ocr_word(engine, img)
        return s, words_arr


m = _i2t()
