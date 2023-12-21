#!/usr/bin/env python3

"""
Compare Botan with OpenSSL using their respective benchmark utils

(C) 2017,2022 Jack Lloyd
    2023 René Meusel, Rohde & Schwarz Cybersecurity GmbH
    2023 René Fischer

Botan is released under the Simplified BSD License (see license.txt)

TODO
 - Output pretty graphs with matplotlib
"""

import logging
import os
import sys
import optparse # pylint: disable=deprecated-module
import subprocess
import re
import json

def setup_logging(options):
    if options.verbose:
        log_level = logging.DEBUG
    elif options.quiet:
        log_level = logging.WARNING
    else:
        log_level = logging.INFO

    class LogOnErrorHandler(logging.StreamHandler):
        def emit(self, record):
            super().emit(record)
            if record.levelno >= logging.ERROR:
                sys.exit(1)

    lh = LogOnErrorHandler(sys.stdout)
    lh.setFormatter(logging.Formatter('%(levelname) 7s: %(message)s'))
    logging.getLogger().addHandler(lh)
    logging.getLogger().setLevel(log_level)

def run_command(cmd):
    logging.debug("Running '%s'", ' '.join(cmd))

    proc = subprocess.Popen(cmd,
                            stdout=subprocess.PIPE,
                            stderr=subprocess.PIPE,
                            universal_newlines=True)
    stdout, stderr = proc.communicate()

    if proc.returncode != 0:
        logging.error("Running command %s failed ret %d", ' '.join(cmd), proc.returncode)

    return stdout + stderr

def get_openssl_version(openssl):
    output = run_command([openssl, 'version'])

    openssl_version_re = re.compile(r'OpenSSL ([0-9a-z\.]+) .*')

    match = openssl_version_re.match(output)

    if match:
        return match.group(1)
    else:
        logging.warning("Unable to parse OpenSSL version output %s", output)
        return output

def get_botan_version(botan):
    return run_command([botan, 'version']).strip()

EVP_MAP = {
    'AES-128/GCM': 'aes-128-gcm',
    'AES-256/GCM': 'aes-256-gcm',
    'ChaCha20': 'chacha20',
    'SHA-1': 'sha1',
    'SHA-256': 'sha256',
    'SHA-384': 'sha384',
    'SHA-512': 'sha512',
    'SHA-3(256)': 'sha3-256',
    'SHA-3(512)': 'sha3-512'
    }

SIGNATURE_EVP_MAP = {
    'RSA': 'rsa',
    'ECDSA': 'ecdsa'
    }

KEY_AGREEMENT_EVP_MAP = {
    'DH': 'ffdh',
    'ECDH': 'ecdh'
    }

def run_openssl_bench(openssl, algo):

    logging.info('Running OpenSSL benchmark for %s', algo)

    cmd = [openssl, 'speed', '-seconds', '1', '-mr']

    if algo in EVP_MAP:
        cmd += ['-evp', EVP_MAP[algo]]
    elif algo in SIGNATURE_EVP_MAP:
        cmd += [SIGNATURE_EVP_MAP[algo]]
    elif algo in KEY_AGREEMENT_EVP_MAP:
        cmd += [KEY_AGREEMENT_EVP_MAP[algo]]
    else:
        cmd += [algo]

    output = run_command(cmd)
    results = []

    if algo in EVP_MAP:
        buf_header = re.compile(r'\+DT:([a-zA-Z0-9-]+):([0-9]+):([0-9]+)$')
        res_header = re.compile(r'\+R:([0-9]+):[a-zA-Z0-9-]+:([0-9]+\.[0-9]+)$')
        ignored = re.compile(r'\+(H|F):.*')

        result = {}

        for l in output.splitlines():
            if ignored.match(l):
                continue

            if not result:
                match = buf_header.match(l)
                if match is None:
                    logging.error("Unexpected output from OpenSSL %s", l)

                result = {'algo': algo, 'buf_size': int(match.group(3))}
            else:
                match = res_header.match(l)

                result['bytes'] = int(match.group(1)) * result['buf_size']
                result['runtime'] = float(match.group(2))
                result['bps'] = int(result['bytes'] / result['runtime'])
                results.append(result)
                result = {}
    elif algo in SIGNATURE_EVP_MAP:
        signature_ops = re.compile(r'\+(R1|R7|R5):([0-9]+):([0-9]+):([0-9]+\.[0-9]+)$')
        verify_ops    = re.compile(r'\+(R2|R8|R6):([0-9]+):([0-9]+):([0-9]+\.[0-9]+)$')
        ignored = re.compile(r'\+(DTP|F2|R3|R4|F4):.*')

        result = {}

        for l in output.splitlines():
            if ignored.match(l):
                continue

            if match := signature_ops.match(l):
                results.append({
                    'algo': algo,
                    'key_size': int(match.group(3)),
                    'op': 'sign',
                    'ops': int(match.group(2)),
                    'runtime': float(match.group(4))})
            elif match := verify_ops.match(l):
                results.append({
                    'algo': algo,
                    'key_size': int(match.group(3)),
                    'op': 'verify',
                    'ops': int(match.group(2)),
                    'runtime': float(match.group(4))
                })
            else:
                logging.error("Unexpected output from OpenSSL %s", l)

    elif algo in KEY_AGREEMENT_EVP_MAP:
        res_header    = re.compile(r'\+(R7|R9|R12|R14):([0-9]+):([0-9]+):([0-9]+\.[0-9]+)$')
        ignored = re.compile(r'\+(DTP|F5|F8):.*')

        result = {}

        for l in output.splitlines():
            if ignored.match(l):
                continue

            if match := res_header.match(l):
                results.append({
                    'algo': algo,
                    'key_size': int(match.group(3)),
                    'ops': int(match.group(2)),
                    'runtime': float(match.group(4))})
            else:
                logging.error("Unexpected output from OpenSSL %s", l)

    return results

def run_botan_bench(botan, runtime, buf_sizes, algo):

    logging.info('Running Botan benchmark for %s', algo)

    cmd = [botan, 'speed', '--format=json', '--msec=%d' % int(runtime * 1000),
           '--buf-size=%s' % (','.join([str(i) for i in buf_sizes])), algo]
    output = run_command(cmd)
    output = json.loads(output)

    return output

def run_botan_signature_bench(botan, runtime, algo):

    logging.info('Running Botan benchmark for %s', algo)

    cmd = [botan, 'speed', '--format=json', '--msec=%d' % int(runtime * 1000), algo]
    output = run_command(cmd)
    output = json.loads(output)

    results = []
    for verify in output:
        for sign in output:
            if sign['op'] == 'sign' and verify['op'] == 'verify' and verify['algo'] == sign['algo']:
                results.append({
                    'algo': algo,
                    'key_size': int(re.search(r'[A-Z]+-[a-z]*([0-9]+).*', sign['algo']).group(1)),
                    'op': 'sign',
                    'ops': sign['events'],
                    'runtime': sign['nanos'] / 1000 / 1000 / 1000,
                })
                results.append({
                    'algo': algo,
                    'key_size': int(re.search(r'[A-Z]+-[a-z]*([0-9]+).*', sign['algo']).group(1)),
                    'op': 'verify',
                    'ops': verify['events'],
                    'runtime': verify['nanos'] / 1000 / 1000 / 1000,
                })
    return results

def run_botan_key_agreement_bench(botan, runtime, algo):

    logging.info('Running Botan benchmark for %s', algo)

    cmd = [botan, 'speed', '--format=json', '--msec=%d' % int(runtime * 1000), algo]
    output = run_command(cmd)
    output = json.loads(output)

    results = []
    for l in output:
        if l['op'] == 'key agreements':
            results.append({
                'algo': algo,
                'key_size': int(re.search(r'[A-Z]+-[a-z]*([0-9]+).*', l['algo']).group(1)),
                'ops': l['events'],
                'runtime': l['nanos'] / 1000 / 1000 / 1000,
            })
    return results

class BenchmarkResult:
    def __init__(self, algo, sizes, openssl_results, botan_results):
        self.algo = algo
        self.results = {}

        def find_result(results, sz):
            for r in results:
                if 'buf_size' in r and r['buf_size'] == sz:
                    return r['bps']
            raise Exception("Could not find expected result in data")

        for size in sizes:
            self.results[size] = {
                'openssl': find_result(openssl_results, size),
                'botan': find_result(botan_results, size)
            }

    def result_string(self):

        out = ""
        for (k, v) in self.results.items():

            if v['openssl'] > v['botan']:
                winner = 'openssl'
                ratio = float(v['openssl']) / v['botan']
            else:
                winner = 'botan'
                ratio = float(v['botan']) / v['openssl']

            out += "algo %s buf_size % 6d botan % 12d bps openssl % 12d bps adv %s by %.02f\n" % (
                self.algo, k, v['botan'], v['openssl'], winner, ratio)
        return out

class SignatureBenchmarkResult:
    def __init__(self, algo, sizes, openssl_results, botan_results):
        self.algo = algo
        self.results = {}

        def find_result(results, sz):
            for verify in results:
                for sign in results:
                    if sign['op'] == 'sign' and verify['op'] == 'verify' and verify['key_size'] == sz:
                        return {'sign': sign['ops'], 'verify': verify['ops']}
            raise Exception("Could not find expected result in data")

        for size in sizes:
            self.results[size] = {
                'openssl': find_result(openssl_results, size),
                'botan': find_result(botan_results, size)
            }

    def result_string(self):

        out = ""
        for (k, v) in self.results.items():
            openssl = v['openssl']
            botan = v['botan']

            for op in openssl.keys():
                if openssl[op] > botan[op]:
                    winner = 'openssl'
                    ratio = float(openssl[op]) / botan[op]
                else:
                    winner = 'botan'
                    ratio = float(botan[op]) / openssl[op]

                out += "algo %s key_size % 5d % 8s botan % 10d openssl % 10d adv %s by %.02f\n" % (
                        self.algo, k, op, botan[op], openssl[op], winner, ratio)

        return out

class KeyAgreementBenchmarkResult:
    def __init__(self, algo, sizes, openssl_results, botan_results):
        self.algo = algo
        self.results = {}

        def find_result(results, sz):
            for r in results:
                if 'key_size' in r and r['key_size'] == sz:
                    return r['ops']
            raise Exception("Could not find expected result in data")

        for size in sizes:
            self.results[size] = {
                'openssl': find_result(openssl_results, size),
                'botan': find_result(botan_results, size)
            }

    def result_string(self):

        out = ""
        for (k, v) in self.results.items():

            if v['openssl'] > v['botan']:
                winner = 'openssl'
                ratio = float(v['openssl']) / v['botan']
            else:
                winner = 'botan'
                ratio = float(v['botan']) / v['openssl']

            out += "algo %s key_size % 6d botan % 12d key agreements openssl % 12d key agreements adv %s by %.02f\n" % (
                self.algo, k, v['botan'], v['openssl'], winner, ratio)
        return out

def bench_algo(openssl, botan, algo):
    openssl_results = run_openssl_bench(openssl, algo)

    buf_sizes = sorted([x['buf_size'] for x in openssl_results])
    runtime = sum(x['runtime'] for x in openssl_results) / len(openssl_results) / len(buf_sizes)

    botan_results = run_botan_bench(botan, runtime, buf_sizes, algo)

    return BenchmarkResult(algo, buf_sizes, openssl_results, botan_results)

def bench_signature_algo(openssl, botan, algo):
    openssl_results = run_openssl_bench(openssl, algo)

    runtime = sum(x['runtime'] for x in openssl_results) / len(openssl_results)
    botan_results = run_botan_signature_bench(botan, runtime, algo)

    kszs_ossl = {x['key_size'] for x in openssl_results}
    kszs_botan = {x['key_size'] for x in botan_results}

    return SignatureBenchmarkResult(algo, sorted(kszs_ossl.intersection(kszs_botan)), openssl_results, botan_results)

def bench_key_agreement_algo(openssl, botan, algo):
    openssl_results = run_openssl_bench(openssl, algo)

    runtime = sum(x['runtime'] for x in openssl_results) / len(openssl_results)
    botan_results = run_botan_key_agreement_bench(botan, runtime, algo)

    kszs_ossl = {x['key_size'] for x in openssl_results}
    kszs_botan = {x['key_size'] for x in botan_results}

    return KeyAgreementBenchmarkResult(algo, sorted(kszs_ossl.intersection(kszs_botan)), openssl_results, botan_results)


def main(args=None):
    if args is None:
        args = sys.argv

    usage = "usage: %prog [options] [algo1] [algo2] [...]"
    parser = optparse.OptionParser(usage=usage)

    parser.add_option('--verbose', action='store_true', default=False, help="be noisy")
    parser.add_option('--quiet', action='store_true', default=False, help="be very quiet")

    parser.add_option('--openssl-cli', metavar='PATH',
                      default='/usr/bin/openssl',
                      help='Path to openssl binary (default %default)')

    parser.add_option('--botan-cli', metavar='PATH',
                      default='/usr/bin/botan',
                      help='Path to botan binary (default %default)')

    (options, args) = parser.parse_args(args)

    setup_logging(options)

    openssl = options.openssl_cli
    botan = options.botan_cli

    if os.access(openssl, os.X_OK) is False:
        logging.error("Unable to access openssl binary at %s", openssl)

    if os.access(botan, os.X_OK) is False:
        logging.error("Unable to access botan binary at %s", botan)

    openssl_version = get_openssl_version(openssl)
    botan_version = get_botan_version(botan)

    logging.info("Comparing Botan %s with OpenSSL %s", botan_version, openssl_version)

    if len(args) > 1:
        algos = args[1:]
        for algo in algos:
            if algo in EVP_MAP:
                result = bench_algo(openssl, botan, algo)
                print(result.result_string())
            elif algo in SIGNATURE_EVP_MAP:
                result = bench_signature_algo(openssl, botan, algo)
                print(result.result_string())
            elif algo in KEY_AGREEMENT_EVP_MAP:
                result = bench_key_agreement_algo(openssl, botan, algo)
                print(result.result_string())
            else:
                logging.error("Unknown algorithm '%s'", algo)
    else:
        for algo in sorted(EVP_MAP):
            result = bench_algo(openssl, botan, algo)
            print(result.result_string())

        for algo in sorted(SIGNATURE_EVP_MAP):
            result = bench_signature_algo(openssl, botan, algo)
            print(result.result_string())

        for algo in sorted(KEY_AGREEMENT_EVP_MAP):
            result = bench_key_agreement_algo(openssl, botan, algo)
            print(result.result_string())

    return 0

if __name__ == '__main__':
    sys.exit(main())
