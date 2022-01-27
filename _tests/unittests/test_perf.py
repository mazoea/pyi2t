# coding=utf-8
import os
import unittest
from testhelpers import get_i2t
_this_dir = os.path.dirname(os.path.abspath(__file__))


class Test_perf(unittest.TestCase):

    @unittest.skip("update dirs")
    def test_perf(self):
        """ test_perf """
        from i2t import perf_probe, dir_spec

        bin_dir = os.path.join(_this_dir, '../../../bins')
        config_dir = os.path.join(
            _this_dir, '../../../../bits/lang_dir/configs')
        lang_dir = os.path.join(_this_dir, '../../../../bits/lang_dir')

        dirs = dir_spec(bin_dir=bin_dir, config_dir=config_dir,
                        lang_dir=lang_dir)
        m = get_i2t(dirs)
        f = os.path.join(_this_dir, 'line1.png')
        self.assertTrue(os.path.exists(f))

        img = m.create_image(f)
        print(m.version())
        print(m.t3.name())
        s_1 = None
        for i in range(1000):
            with perf_probe('ocr_line_v4', min_t=0.01, enabled=True, raw_print=True):
                s, words = m.ocr_line_v4(img)
            self.assertTrue(0 < len(s))
            if s_1 is None:
                s_1 = s
                print(' '.join('[%s:%s:%s] ' % (w.conf(), w.text, w.bbox)
                               for w in words))
                continue
            self.assertTrue(s == s_1)


if __name__ == '__main__':
    unittest.main()
