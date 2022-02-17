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

    def test_subtypes_conv(self):
        """ test_subtypes_conv """
        m = get_i2t()

        for exp, inp in (
                ({'dos': {}, 'revCode': {}}, []),
                ({'dos': {}, 'revCode': {'fill': 'up'}},
                 ['revCode_section_bottom']),
                ({'dos': {}, 'revCode': {'fill': 'down'}},
                 ['revCode_section_top']),
                ({'dos': {}, 'revCode': {'fill': 'down'}},
                 ['revCode_section_start']),

            ({'dos': {'fill': 'down'}, 'revCode': {}}, ['dos_section_top']),
            ({'dos': {'fill': 'down'}, 'revCode': {}},
                    ['dos_section_start']),
            ({'dos': {'missing': 'year'}, 'revCode': {}},
                    ['dos_missing_year']),
        ):
            val = m.subtypes_dict(inp)
            print(val)
            self.assertEqual(val, exp)

    def test_is_ib_line(self):
        """ test_is_ib_line """
        m = get_i2t()
        string = "09/18/2021 93010 93010 ROUTINE ELECTROCARDIOGRAM (EKG) USING 1 $115.00"

        val = m.is_ib_line(string)
        print(val)

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

        img.to8bpp(True)
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
