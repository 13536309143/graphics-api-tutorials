"""Compare deterministic Vulkan/DX12 PPM readbacks on the same test machine."""
from pathlib import Path
import argparse
import json

ROOT = Path(__file__).resolve().parents[1]


def ppm(path):
    with path.open('rb') as stream:
        def line():
            value = stream.readline()
            while value.startswith(b'#'):
                value = stream.readline()
            return value.strip()
        if line() != b'P6':
            raise ValueError(f'Expected RGB P6 PPM: {path}')
        width, height = map(int, line().split())
        if line() != b'255':
            raise ValueError('Only 8-bit PPM is supported')
        pixels = stream.read()
    if len(pixels) != width * height * 3:
        raise ValueError(f'Unexpected pixel size: {path}')
    return (width, height), pixels


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--max-mean-error', type=float, default=0.25,
                        help='Average absolute channel difference in 0..255 units')
    parser.add_argument('--json', type=Path, help='Optional output report path')
    args = parser.parse_args()
    reports = []
    for number in (21, 22, 29, 36, 50):
        left = ROOT / f'Vulkan/native/build/bin/Debug/vk_lab_{number}.ppm'
        right = ROOT / f'DirectX12/build/bin/Debug/dx_lab_{number}.ppm'
        if not left.is_file() or not right.is_file():
            parser.error('Run the model, GPU-particle and integration smoke examples first')
        size_a, a = ppm(left)
        size_b, b = ppm(right)
        if size_a != size_b:
            raise ValueError(f'Readback dimensions differ for experiment {number}')
        errors = [abs(x - y) for x, y in zip(a, b)]
        mean = sum(errors) / len(errors)
        reports.append({'experiment': number, 'dimensions': list(size_a),
                        'mean_absolute_channel_error': mean, 'maximum_channel_error': max(errors),
                        'passed': mean <= args.max_mean_error})
    output = {'scope': 'Same machine, Debug smoke time=1, fixed scene; not a performance benchmark',
              'maximum_mean_error': args.max_mean_error, 'comparisons': reports}
    if args.json:
        args.json.write_text(json.dumps(output, indent=2) + '\n', encoding='utf-8')
    for report in reports:
        print(f"{report['experiment']}: mean={report['mean_absolute_channel_error']:.8f}, "
              f"max={report['maximum_channel_error']}, passed={report['passed']}")
    return 0 if all(report['passed'] for report in reports) else 1


if __name__ == '__main__':
    raise SystemExit(main())
