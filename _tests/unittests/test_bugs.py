# coding=utf-8
import json
import os

import unittest
import testhelpers
from testhelpers import get_i2t, test_data_dir


class Test_basic(unittest.TestCase):

    def test_version(self):
        """ test_version """
        m = get_i2t()
        print(m.version())

    def test_ocr3(self):
        """ test_version """
        m = get_i2t()
        f = os.path.join(test_data_dir, 'oneline/line1.png')
        self.assertTrue(os.path.exists(f))

        img = m.create_image(f)
        print(img.hash())
        print(m.t3.name())
        s, words = m.ocr_line_v3(img)
        print('[%s] is_binary: %s' % (s, img.is_binary()))
        print(' '.join('[%s:%s:%s] ' % (w.conf(), w.text, w.bbox)
                       for w in words))

        img.to8bpp()
        s, words = m.ocr_line_v3(img)
        print('[%s] is_binary: %s' % (s, img.is_binary()))
        print(' '.join('[%s:%s:%s] ' % (w.conf(), w.text, w.bbox)
                       for w in words))

        img.binarize_otsu()
        s, words = m.ocr_line_v3(img)
        print('[%s] is_binary: %s' % (s, img.is_binary()))
        print(' '.join('[%s:%s:%s] ' % (w.conf(), w.text, w.bbox)
                       for w in words))

        img.upscale2x()
        s, words = m.ocr_line_v3(img)
        print('[%s] is_binary: %s' % (s, img.is_binary()))
        print(' '.join('[%s:%s:%s] ' % (w.conf(), w.text, w.bbox)
                       for w in words))


if __name__ == '__main__':
    unittest.main()
