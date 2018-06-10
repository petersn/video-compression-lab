#!/usr/bin/python2
# Written to be 2/3 compatible.

import sys, os, subprocess, re

NORMAL = "\033[0m"
RED    = "\033[91m"
GREEN  = "\033[92m"

if len(sys.argv) < 2:
	print("Usage: grading.py path_to_ffmpeg/ [test names...]")
	exit(1)

here = os.path.dirname(os.path.abspath(os.path.realpath(__file__)))
sample1_path = os.path.join(here, "samples", "sample1.mkv")

os.chdir(sys.argv[1])

tests = []
def test(f):
	tests.append(f)

@test
def test_for_ffmpeg_binary():
	if not os.path.exists("ffmpeg"):
		return "ffmpeg binary not present -- did you compile?"

@test
def test_for_labcodec_in_codecs():
	print("Invoking ./ffmpeg -codecs, and checking for labcodec.")
	s = subprocess.check_output(["./ffmpeg", "-codecs"], stderr=open("/dev/null"))
	if b"labcodec" not in s:
		return "labcodec not present in output -- check the hints before task 1"

@test
def test_can_encode():
	print("Invoking ./ffmpeg to encode to your format.")
	s = subprocess.check_output(["./ffmpeg", "-i", sample1_path, "-vcodec", "labcodec", "-f", "null", "/dev/null"], stdin=open("/dev/null"), stderr=open("/dev/null"))

@test
def test_can_decode():
	print("Invoking ./ffmpeg to decode from your format.")
	p1 = subprocess.Popen(["./ffmpeg", "-i", sample1_path, "-vcodec", "labcodec", "-f", "matroska", "-"], stdin=open("/dev/null"), stdout=subprocess.PIPE, stderr=open("/dev/null"))
	p2 = subprocess.Popen(["./ffmpeg", "-y", "-i", "-", "-f", "rawvideo", "-pix_fmt", "rgb24", "/dev/null"], stdin=p1.stdout, stderr=open("/dev/null"))
	p1.stdout.close()
	output = p2.communicate()[0]
	if p2.returncode != 0:
		return "decoding side failed"

def extract_psnr(output):
	psnr_line = output.strip().rsplit(b"\n", 1)[-1]
	if b"PSNR" not in psnr_line:
		raise ValueError("decoding side failed to produce a PSNR value -- possibly a bug in the grader?")
	psnr = float(re.search(b"min:([.0-9]+|inf)", psnr_line).groups()[0])
	return psnr

@test
def test_raw_video_psnr():
	print("Invoking ./ffmpeg to compute a PSNR of your raw video format.")
	p1 = subprocess.Popen(["./ffmpeg", "-i", sample1_path, "-vcodec", "labcodec", "-q:v", "10", "-f", "matroska", "-"], stdin=open("/dev/null"), stdout=subprocess.PIPE, stderr=open("/dev/null"))
	p2 = subprocess.Popen(["./ffmpeg", "-i", "-", "-i", sample1_path, "-filter_complex", "psnr", "-f", "null", "-"], stdin=p1.stdout, stderr=subprocess.PIPE)
	p1.stdout.close()
	output = p2.communicate()[1]
	if p2.returncode != 0:
		return "decoding side failed"
	psnr = extract_psnr(output)
	print("Minimum frame PSNR: %r" % psnr)
	if psnr < 40.0:
		return "insufficient min frame PSNR -- must be at least 40.0 to pass"

@test
def test_compression():
	print("Invoking ./ffmpeg to compute a PSNR and compression ratio of your video format.")
	print("Compressing %s..." % sample1_path)
	compressed_file = subprocess.check_output(["./ffmpeg", "-i", sample1_path, "-vcodec", "labcodec", "-q:v", "5", "-f", "matroska", "-"], stdin=open("/dev/null"), stderr=open("/dev/null"))
	uncompressed_length = 512 * 288 * 3 * 270
	fraction_of_uncompressed = len(compressed_file) / float(uncompressed_length)
	print("Compressed length: %.3f MiB (%.5f%% of raw)" % (len(compressed_file) / 2**20.0, 100.0 * fraction_of_uncompressed))
	p = subprocess.Popen(["./ffmpeg", "-i", "-", "-i", sample1_path, "-filter_complex", "psnr", "-f", "null", "-"], stdin=subprocess.PIPE, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
	output = p.communicate(compressed_file)[1]
	if p.returncode != 0:
		return "decoding side failed"
	psnr = extract_psnr(output)
	print("Minimum frame PSNR: %.3f" % psnr)
	print("Compression lab final score is: %r points" % int(psnr / fraction_of_uncompressed))
	if fraction_of_uncompressed > 0.05:
		return "compressed size must be less than 5% of uncompressed size"
	if psnr < 35:
		return "PSNR must be at least 35.0"
	print("Congratulations! You have completed the compression challenge.")

limit_to = sys.argv[2:]

print("Running tests...")

score = 0
for index, test in enumerate(tests):
	print("") # Quotes for Python 2 compatibility.

	func_name = test.func_name if hasattr(test, "func_name") else test.__qualname__
	name = "%s (%i/%i)" % (func_name, index + 1, len(tests))
	if limit_to and func_name not in limit_to:
		print("Skipping: %s" % name)
		continue

	print("=== %s" % name)

	try:	
		result = test()
	except Exception as e:
		print(RED + "Exception raised: %r" % e + NORMAL)
		continue
	if result is None:
		print(GREEN + "Passed." + NORMAL)
		score += 1
	else:
		print(RED + "Failed: %s" % (result,) + NORMAL)

print("\nTotal score: %i / %i" % (score, len(tests)))

if score >= len(tests):
	print(GREEN + "All tests passed!" + NORMAL)
else:
	print(RED + "Not all tests passed." + NORMAL)

