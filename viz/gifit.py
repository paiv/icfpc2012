#!/usr/bin/env python3
import os
import shutil
import tempfile

from glob import glob
from imagemagick import ImageMagick
from PIL import Image


class GameBoard:
    valid = set(' .#*\\LOR')

    def __init__(self):
        self.width = None
        self.complete = True
        self.lines = []

    @property
    def height(self):
        return len(self.lines)

    def read_line(self, line):
        line_valid = len(set(line) - self.valid) == 0
        if line_valid and self.width is not None:
            line_valid = len(line) == self.width

        if not line_valid:
            self.complete = True
        else:
            if self.complete:
                self.lines = []
            self.complete = False
            self.lines.append(line)
            if self.width is None:
                self.width = len(line)

    def __str__(self):
        return 'GameBoard ({})'.format('{}x{}'.format(self.width, len(self.lines)) if len(self.lines) > 0 else 'blank')


class Painter:
    tile_names = {
        ' ': 'empty',
        '.': 'earth',
        '#': 'wall',
        '*': 'rock',
        '\\': 'lambda',
        'L': 'lift',
        'O': 'openlift',
        'R': 'robot',
    }
    tile_ext = '.png'

    def __init__(self, tiles=None):
        self.tiles = self._cache_tiles(tiles)
        self.tile_size = self.tiles['#'].size

    def _cache_tiles(self, from_dir):
        tiles = dict()
        for key,name in self.tile_names.items():
            fn = os.path.join(from_dir, name + self.tile_ext)
            tiles[key] = Image.open(fn, 'r')
        return tiles

    def paint(self, board, target=None, format='png'):
        if hasattr(target, 'write'):
            self._write_file(target, board, format=format)
        else:
            dirs = os.path.dirname(target)
            os.makedirs(dirs, exist_ok=True)

            with open(target, 'wb') as fd:
                self._write_file(fd, board, format=format)

    def _write_file(self, fd, board, format=None):
        xstride,ystride = self.tile_size
        w = board.width * xstride
        h = board.height * ystride
        with Image.new(mode='RGBA', size=(w,h)) as img:
            row = 0
            for line in board.lines:
                col = 0
                for c in line:
                    tile = self.tiles[c]
                    img.paste(tile, (col,row))
                    col += xstride
                row += ystride
            img.save(fd)


class LogAnimator:

    def __init__(self, fps=1.0, loop=False, save_frames=None, max_frames=None, silent=False, tiles=None):
        self.fps = fps
        self.loop = loop
        self.save_frames = save_frames
        self.max_frames = max_frames or 0
        self.silent = silent
        self.frame_count = 0
        self.painter = Painter(tiles=tiles)

    def process(self, logfile, target):

        if hasattr(logfile, 'read'):
            self._process_file(logfile, target)
        else:
            with open(logfile, 'r') as fd:
                self._process_file(fd, target, title=logfile)

    def _process_file(self, infile, target, title=None):
        with tempfile.TemporaryDirectory() as tempdir:
            frame_file = os.path.join(tempdir, 'frame.png')

            self._generate_frames(infile, frame_file)

            if self.frame_count > 0:
                self._animate(tempdir, target)
            elif not self.silent:
                sys.stderr.write('! no frames\n')

            if self.save_frames:
                self._move_files(tempdir, self.save_frames)

    def _move_files(self, source_dir, target_dir):
        os.makedirs(target_dir, exist_ok=True)
        for fn in glob(os.path.join(source_dir, '*')):
            shutil.move(fn, target_dir)

    def _generate_frames(self, infile, target):
        board = GameBoard()
        for line in infile.readlines():
            line = line.strip('\n')

            if board.complete:
                board.read_line(line)
            else:
                board.read_line(line)
                if board.complete:
                    if self.frame_count == 0:
                        # pause at first frame
                        self._export_frame(board, target)
                        self._export_frame(board, target)
                        self._export_frame(board, target)

                    self._export_frame(board, target)

        if self.frame_count > 0:
            # pause at last frame
            self._export_frame(board, target)
            self._export_frame(board, target)
            self._export_frame(board, target)

    def _export_frame(self, board, target):
        if self.max_frames > 0 and self.frame_count >= self.max_frames:
            return

        frameno = self.frame_count
        self.frame_count += 1

        base,ext = os.path.splitext(target)
        fn = '{}{:06}{}'.format(base, frameno, ext)

        if not self.silent:
            sys.stderr.write('.')
            sys.stderr.flush()

        self.painter.paint(board, fn, format='png')

    def _animate(self, from_dir, target):
        if not self.silent:
            sys.stderr.write('♡\n')
            sys.stderr.flush()

        delay = round(10000 / self.fps) / 100
        loop = 0 if self.loop else 1

        img = ImageMagick('+fuzz', layers='optimize-transparency', alpha='set')
        img.animate(from_dir, target, loop=loop, delay=delay)


def do_viz(logfile, target, **kwargs):
    anim = LogAnimator(**kwargs)
    anim.process(logfile, target)


if __name__ == '__main__':

    import argparse
    import sys

    import __main__
    default_tiles = os.path.join(os.path.dirname(__main__.__file__), 'tiles')


    parser = argparse.ArgumentParser(description='Log Animator')

    parser.add_argument('logfile', nargs='?', type=argparse.FileType(mode='r'),
        default=sys.stdin,
        help='source log file')

    parser.add_argument('target', nargs='?', type=argparse.FileType(mode='w'),
        default=sys.stdout.buffer,
        help='target gif file')

    parser.add_argument('-f', '--fps', metavar='FPS', type=float, default=5.0,
        help='animation frames per second')

    parser.add_argument('-l', '--loop', action='store_true',
        help='loop animation')

    parser.add_argument('-n', '--max-frames', metavar='LIMIT', type=int, default=None,
        help='stop generating after N frames')

    parser.add_argument('-p', '--save-frames', metavar='DIR', type=str, default=None,
        help='directory to save frame images')

    parser.add_argument('-s', '--silent', action='store_true',
        help='be quiet')

    parser.add_argument('-t', '--tiles', metavar='TILES', type=str, default=default_tiles,
        help='directory of tile images')

    args = parser.parse_args()


    do_viz(**args.__dict__)
