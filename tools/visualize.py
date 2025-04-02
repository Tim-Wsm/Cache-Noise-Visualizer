#!/usr/bin/env python3
# -*- coding: utf-8 -*-

from math import ceil

import h5py
import numpy
import time
import matplotlib.pyplot as plt
import matplotlib.animation as animation

from matplotlib.container import BarContainer
from matplotlib.image import AxesImage

from functools import reduce
from multiprocessing import Process, cpu_count, Queue


def image_worker(file_name, core_id, combines_lines, queue):
    """
    Worker funktion, that calculates a part of the image.
    """
    measurement = MeasureData.from_file(
        file_name,
        combines=1,
        combine_all=False,
        combine_lines=combines_lines,
    )
    start = ceil(len(measurement) / cpu_count()) * core_id
    end = ceil(len(measurement) / cpu_count()) * (core_id + 1)
    if len(measurement) <= end:
        end = end - (end - len(measurement))

    if combines_lines:
        result = numpy.zeros(len(measurement[0]))
    else:
        result = numpy.zeros((len(measurement[0]), len(measurement[0][0])))

    for i in range(start, end):
        result = result + measurement[i]

    queue.put(result)


def generate_image(file_name, combine_lines):
    """
    Calculates the content of an image in multiple processes.
    """
    queue = Queue()
    for i in range(cpu_count()):
        p = Process(
            target=image_worker, args=(file_name, i, combine_lines, queue))
        p.start()

    result = queue.get()
    for i in range(1, cpu_count()):
        result += queue.get()

    return result


def video_worker(args, core_id, queue):
    """
    Workerfunction that calculates part of min_max_mean and
    appends the result in the queue.
    """
    measurement = MeasureData.from_file(
        args.measure_data,
        combines=args.combine,
        combine_all=args.type == 'image',
        combine_lines=args.graph == 'bar_chart',
    )
    start = ceil(len(measurement) / cpu_count()) * core_id
    end = ceil(len(measurement) / cpu_count()) * (core_id + 1)
    if len(measurement) <= end:
        end = end - (end - len(measurement))

    result = []
    for i in range(start, end):
        if measurement.combine_lines:
            result.extend(measurement[i])
        else:
            result.extend(measurement[i].flatten())

    minimum = result[0]
    maximum = result[0]
    summary = result[0]

    for value in result:
        if value < minimum:
            minimum = value
        if maximum < value:
            maximum = value
        summary += value

    queue.put((maximum, minimum, summary, len(result)))


def print_min_max_mean(args):
    """
    Calculates and prints the minimum, maximum and mean in multiple processes.
    """
    if args.type == 'video':
        queue = Queue()
        for i in range(cpu_count()):
            p = Process(target=video_worker, args=(args, i, queue))
            p.start()

        maximum, minimum, summary, value_count = queue.get()

        for i in range(1, cpu_count()):
            new_maximum, new_minimum, new_summary, new_value_count = queue.get(
            )
            if maximum < new_maximum:
                maximum = new_maximum
            if new_minimum < minimum:
                minimum = new_minimum
            summary += new_summary
            value_count += new_value_count

        mean = summary / value_count

    else:
        image = generate_image(args.measure_data, args.graph == 'bar_chart')
        values = image.flatten()
        maximum = values.max()
        minimum = values.min()
        mean = values.mean()

    print('maximum: {}'.format(maximum))
    print('minimum: {}'.format(minimum))
    print('mean: {}'.format(mean))


class MeasureData:
    """
    Wrapper Class for a hdf5 file, which can combine rows and/or columns
    """

    def __init__(
            self,
            file,
            combines,
            combine_lines,
    ):
        self.file = file
        self.iteration = 0
        self.combines = combines
        self._combine_lines = combine_lines

    def __iter__(self):
        return self

    def __next__(self):
        """
        Returns the next chunk from the reader.

        Returns
        -------
            A chunk of
        """
        try:
            self.iteration += 1
            return self[self.iteration - 1]
        except KeyError:
            raise StopIteration

    def next(self):
        """
        Wrapper for __next__.
        """
        return self.__next__()

    def __len__(self):
        return ceil(len(self.file) / self.combines)

    def __getitem__(self, key):
        """
        Returns the a chunk from the reader at position key and combines the
        underlying data.

        Returns
        -------
            A chunk of
        """
        if self.combines == 1:
            return self.chunk_at(key)
        else:
            if len(self) < key:
                raise KeyError

            if len(self) == 1:  # case if an image gets created
                # the child processes can not open the file if the file is
                # still open in the parent process
                file_name = self.file.filename
                self.file.close()
                time.sleep(0.1)
                return generate_image(file_name, self.combine_lines)

            elif len(self) == key and len(self.file) % self.combines:
                max_combines = len(self.file) % self.combines
            else:
                max_combines = self.combines

            data = list(
                map(
                    lambda n: self.chunk_at(key + n),
                    range(0, max_combines),
                ))
            # combine all blocks with the combine function
            return reduce(
                lambda l, r: list(map(lambda e: e[0] + e[1], zip(l, r))), data)

    def chunk_at(self, key):
        chunk = self.file[str(key)]
        if self._combine_lines:
            # fold a single row into a single value
            return list(
                map(lambda row: reduce(lambda x, y: x.astype(numpy.uint64) + y.astype(numpy.uint64), row),
                    chunk))
        else:
            return chunk.value.astype(numpy.uint64)

    @property
    def combine_lines(self):
        return self._combine_lines

    @classmethod
    def from_file(
            cls,
            path,
            combines=1,
            combine_all=False,
            combine_lines=False,
    ):
        """
        Arguments
        ---------
            path: Path to hdf5 file.
            chunks_size: Amount of blocks which will be combined.
            combine_function: The function which combines the elementes of
                    chunk_size big array.
            skips: The amount of blocks which will be skipped at the beginning.

        """
        if combines <= 0:
            raise ValueError('combines can not be lower than one.')

        print('Opening data set file ...')

        file = h5py.File(path, 'r')

        if combine_all:
            combines = len(file.items())

        return cls(file, combines, combine_lines)


def parse():
    """
    Parses arguments from the command line and returns them.

    Returns
    -------
        Arguments
    """
    import argparse
    parser = argparse.ArgumentParser(
        formatter_class=argparse.ArgumentDefaultsHelpFormatter)

    parser.add_argument(
        'measure_data',
        type=str,
        help='Input file with measure data.',
    )

    parser.add_argument(
        '-o',
        '--output',
        type=str,
        metavar='FILE',
        default=None,
        help='Output file for the animation/image.',
    )

    parser.add_argument(
        '-g',
        '--graph',
        type=str,
        choices=['bar_chart', 'heatmap'],
        default='bar_chart',
        help=
        'The type of graph in which the measurment data will be displayed.',
    )

    parser.add_argument(
        '-q',
        '--quality',
        metavar='DPI',
        type=int,
        default=100,
        help='The amount of DPI.',
    )

    parser.add_argument(
        '-t',
        '--type',
        type=str,
        choices=['video', 'image'],
        default='video',
        help='Type of the output file.',
    )

    parser.add_argument(
        '-s',
        '--stats',
        action='store_true',
        help='Displays information about the measure data.',
    )

    parser.add_argument(
        '-c',
        '--combine',
        metavar='AMOUNT',
        default=1,
        type=int,
        help='Defines the amount of iterations which will be combined.',
    )

    parser.add_argument(
        '--max',
        metavar='MAX',
        default=None,
        type=int,
        help='Sets the hightest value that will be displayed.',
    )

    parser.add_argument(
        '--min',
        metavar='MIN',
        default=None,
        type=int,
        help='Sets the lowest value that will be displayed.',
    )

    return parser.parse_args()


def generic_update(new_data, container):
    """
    Update function for the antimation.
    For more information see pandas -> AnimationFunc
    """
    if isinstance(container, BarContainer):
        for data, artist in zip(new_data, container):
            artist.set_height(data)
        return container
    elif isinstance(container, AxesImage):
        container.set_data(new_data)
        return [container]


def generic_video(measurement, args, figure, chart):
    if args.type == 'video':
        map_animation = animation.FuncAnimation(
            figure,
            generic_update,
            frames=measurement,
            fargs=(chart, ),
            interval=50,
            repeat=False,
            blit=True,
        )

        if args.output is not None:
            print('Writing video to file {} (This will take some time)'.format(
                args.output))
            map_animation.save(args.output, dpi=args.quality)
        else:
            plt.show()
    elif args.type == 'image':
        if args.output is not None:
            print('Writing image to file {}'.format(args.output))
            figure.savefig(args.output, dpi=args.quality)
        else:
            plt.show()
    else:
        raise ValueError('Not allowed type ({}).'.format(args.type))


def heatmap_plot(measurement, args):
    """
    This function will handle the heatmap plot.
    """
    fig = plt.figure()
    first_block = measurement.next()

    heatmap = plt.imshow(
        first_block,
        aspect='auto',
        vmin=args.min,
        vmax=args.max,
    )

    cbar = plt.colorbar(format='%d')
    cbar.ax.get_yaxis().labelpad = 15
    cbar.ax.set_ylabel('# of cpu cycles', rotation=270)

    plt.title('cache noise (combined {} iteration(s))'.format(
        measurement.combines), )
    plt.xlabel('Cache Lines')
    plt.ylabel('Cache Sets')
    generic_video(measurement, args, fig, heatmap)


def bar_chart_plot(measurement, args):
    """
    This function will handle the bar_chart plot.
    """
    fig = plt.figure()
    first_block = measurement.next()

    if args.max:
        init_block = [args.max for _ in range(len(first_block))]
    else:
        init_block = first_block

    bar_chart = plt.bar(
        x=range(len(first_block)),
        height=init_block,
        capsize=args.max,
    )
    bar_chart.height = first_block

    plt.title('cache noise (combined {} iteration(s))'.format(
        measurement.combines))
    plt.ylabel('Clock Cycles')
    plt.xlabel('Cache Sets')

    generic_video(measurement, args, fig, bar_chart)


def main():
    """
    Main function
    """
    args = parse()

    if args.stats:
        print_min_max_mean(args)
        print('data sets: {}'.format(len(h5py.File(args.measure_data, 'r'))))
        return

    measurement = MeasureData.from_file(
        args.measure_data,
        combines=args.combine,
        combine_all=args.type == 'image',
        combine_lines=args.graph == 'bar_chart',
    )

    if args.graph == 'heatmap':
        heatmap_plot(measurement, args)
    elif args.graph == 'bar_chart':
        bar_chart_plot(measurement, args)
    else:
        raise ValueError('The graph type `{}` is not supported.', args.graph)


if __name__ == '__main__':
    main()
