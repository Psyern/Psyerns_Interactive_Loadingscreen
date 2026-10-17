"""
Psyerns Interactive Loadingscreen - procedural layer generator.

Renders the default parallax scene (dusk over a ruined Chernarus-style town)
as separate transparent layers and writes them as Enfusion .edds
(BGRA8 + LZ4, power-of-two sizes) into the mod's gui/textures folder.
PNG previews land in Psyerns_Interactive_Loadingscreen_Doks/previews.

Usage:
    python -m pip install pillow lz4 numpy
    python psy_ils_generate_textures.py

Replace any layer with your own artwork: export a PNG with alpha, power-of-two
size (2048x1024 recommended) and convert it with encode_edds() below
(python psy_ils_generate_textures.py --convert my.png out.edds).
"""
import math
import os
import random
import struct
import sys

import lz4.block
import numpy as np
from PIL import Image, ImageDraw, ImageFilter

HERE = os.path.dirname(os.path.abspath(__file__))
REPO = os.path.dirname(os.path.dirname(HERE))
OUT_DIR = os.path.join(REPO, 'Psyerns_Interactive_Loadingscreen', 'gui', 'textures')
PREVIEW_DIR = os.path.join(REPO, 'Psyerns_Interactive_Loadingscreen_Doks', 'previews')

W = 2048
H = 1024
SEED = 1337

# ---------- Enfusion .edds encoder (BGRA8, LZ4 mips, smallest mip first) ----------

CHUNK = 65536
COPY_MAX = 64


def mip_count(w, h):
	n = 1
	while w > 1 or h > 1:
		w = max(1, w // 2)
		h = max(1, h // 2)
		n += 1
	return n


def encode_lz4_block(rawbytes):
	total = len(rawbytes)
	out = bytearray(struct.pack('<I', total))
	pos = 0
	while pos < total:
		chunk = rawbytes[pos:pos + CHUNK]
		pos += len(chunk)
		comp = lz4.block.compress(chunk, mode='high_compression', store_size=False)
		hdr = len(comp)
		if pos >= total:
			hdr |= 0x80000000
		out += struct.pack('<I', hdr)
		out += comp
	return bytes(out)


def is_pot(v):
	return v > 0 and (v & (v - 1)) == 0


def decode_edds(path):
	"""Reads an uncompressed BGRA8 .edds (COPY or LZ4 mips) and returns the top mip as RGBA image."""
	data = open(path, 'rb').read()
	if data[:4] != b'DDS ':
		raise ValueError(f'{path}: not a DDS/EDDS file')
	if data[84:88] != b'\0\0\0\0':
		raise ValueError(f'{path}: block-compressed ({data[84:88]}) EDDS is not supported, convert from the source image')
	height = struct.unpack_from('<I', data, 12)[0]
	width = struct.unpack_from('<I', data, 16)[0]
	mips = struct.unpack_from('<I', data, 28)[0]
	off = 128
	blocks = []
	for _ in range(mips):
		blocks.append((data[off:off + 4], struct.unpack_from('<I', data, off + 4)[0]))
		off += 8
	pos = off
	for tag, size in blocks[:-1]:
		pos += size
	tag, size = blocks[-1]
	raw = data[pos:pos + size]
	expected = width * height * 4
	if tag == b'COPY':
		pixels = raw[:expected]
	else:
		total = struct.unpack_from('<I', raw, 0)[0]
		p = 4
		out = bytearray()
		while p + 4 <= len(raw) and len(out) < total:
			hdr = struct.unpack_from('<I', raw, p)[0]
			p += 4
			csize = hdr & 0x7FFFFFFF
			chunk = raw[p:p + csize]
			p += csize
			usize = min(CHUNK, total - len(out))
			if out:
				out += lz4.block.decompress(chunk, uncompressed_size=usize, dict=bytes(out[-CHUNK:]))
			else:
				out += lz4.block.decompress(chunk, uncompressed_size=usize)
		pixels = bytes(out[:expected])
	return Image.frombytes('RGBA', (width, height), pixels, 'raw', 'BGRA')


def encode_edds(img, path):
	img = img.convert('RGBA')
	width, height = img.size
	if not is_pot(width) or not is_pot(height):
		raise ValueError(f'{path}: {width}x{height} is not power-of-two (DayZ renders NPOT UI textures as white blocks)')
	mips = mip_count(width, height)
	sizes = []
	w, h = width, height
	for _ in range(mips):
		sizes.append((w, h))
		w = max(1, w // 2)
		h = max(1, h // 2)
	sizes.reverse()
	blocks = []
	for (mw, mh) in sizes:
		if (mw, mh) == (width, height):
			mip = img
		else:
			mip = img.resize((mw, mh), Image.LANCZOS)
		raw = mip.tobytes('raw', 'BGRA')
		if len(raw) <= COPY_MAX:
			blocks.append((b'COPY', raw))
		else:
			blocks.append((b'LZ4 ', encode_lz4_block(raw)))
	hdr = bytearray(128)
	hdr[0:4] = b'DDS '
	struct.pack_into('<I', hdr, 4, 124)
	struct.pack_into('<I', hdr, 8, 0x0002100F)
	struct.pack_into('<I', hdr, 12, height)
	struct.pack_into('<I', hdr, 16, width)
	struct.pack_into('<I', hdr, 20, width * 4)
	struct.pack_into('<I', hdr, 24, 0)
	struct.pack_into('<I', hdr, 28, mips)
	hdr[36:40] = b'ENF1'
	struct.pack_into('<I', hdr, 76, 32)
	struct.pack_into('<I', hdr, 80, 0x41)
	struct.pack_into('<I', hdr, 84, 0)
	struct.pack_into('<I', hdr, 88, 32)
	struct.pack_into('<I', hdr, 92, 0x00FF0000)
	struct.pack_into('<I', hdr, 96, 0x0000FF00)
	struct.pack_into('<I', hdr, 100, 0x000000FF)
	struct.pack_into('<I', hdr, 104, 0xFF000000)
	struct.pack_into('<I', hdr, 108, 0x00401008)
	body = bytearray()
	table = bytearray()
	for tag, payload in blocks:
		table += tag
		table += struct.pack('<I', len(payload))
		body += payload
	with open(path, 'wb') as f:
		f.write(hdr)
		f.write(table)
		f.write(body)

# ---------- helpers ----------


def hex_rgb(h):
	h = h.lstrip('#')
	return tuple(int(h[i:i + 2], 16) for i in (0, 2, 4))


def ridge(rng, width, base, rough, octaves, step=4):
	"""1D fractal ridge line (sum of random sines), returns y per x."""
	xs = np.arange(0, width + step, step, dtype=np.float64)
	ys = np.full_like(xs, float(base))
	amp = rough
	freq = 1.0 / width
	for _ in range(octaves):
		phase = rng.uniform(0, math.tau)
		# integer cycles per width keep the ridge tileable-ish at the edges
		cycles = max(1, round(freq * width * rng.uniform(1.5, 3.0)))
		ys += amp * np.sin(xs / width * math.tau * cycles + phase)
		amp *= 0.5
		freq *= 2.1
	return list(zip(xs.tolist(), ys.tolist()))


def ground_y(pts, x):
	"""Ridge height at x (linear interpolation, clamped to the ridge range)."""
	xs = [p[0] for p in pts]
	ys = [p[1] for p in pts]
	return float(np.interp(x, xs, ys))


def fill_below(draw, pts, color, height):
	poly = [(pts[0][0], height)] + pts + [(pts[-1][0], height)]
	draw.polygon(poly, fill=color)


def pine(draw, x, ground, h, color):
	w = h * 0.42
	tiers = 4
	for i in range(tiers):
		top = ground - h + i * h * 0.18
		bottom = top + h * 0.42
		half = w * (0.45 + i * 0.18) / 2
		draw.polygon([(x, top), (x - half, bottom), (x + half, bottom)], fill=color)
	draw.rectangle([x - h * 0.025, ground - h * 0.15, x + h * 0.025, ground + 2], fill=color)


def vertical_gradient(stops, width, height):
	"""stops: list of (pos 0..1, hex)."""
	ys = np.linspace(0.0, 1.0, height)
	out = np.zeros((height, 3), dtype=np.float64)
	for c in range(3):
		out[:, c] = np.interp(ys, [s[0] for s in stops], [hex_rgb(s[1])[c] for s in stops])
	img = np.repeat(out[:, None, :], width, axis=1)
	return img


def save(img, name):
	edds = os.path.join(OUT_DIR, name + '.edds')
	encode_edds(img, edds)
	img.save(os.path.join(PREVIEW_DIR, name + '.png'))
	print(f'  {name}.edds  {img.size[0]}x{img.size[1]}')

# ---------- layers ----------


def layer_sky(rng):
	sky = vertical_gradient([
		(0.00, '#070a18'),
		(0.30, '#1a1838'),
		(0.52, '#4a2c4f'),
		(0.66, '#a4523e'),
		(0.72, '#e08a4a'),
		(0.80, '#5a3040'),
		(1.00, '#1a1222'),
	], W, H)
	yy, xx = np.mgrid[0:H, 0:W].astype(np.float64)
	# sun glow low on the horizon
	sx, sy = W * 0.62, H * 0.70
	d = np.sqrt(((xx - sx) / 1.6) ** 2 + (yy - sy) ** 2)
	glow = np.exp(-(d / 260.0) ** 2)
	sky += glow[..., None] * np.array([255, 170, 90]) * 0.55
	# moon
	mx, my = W * 0.27, H * 0.22
	dm = np.sqrt((xx - mx) ** 2 + (yy - my) ** 2)
	sky += np.exp(-(dm / 160.0) ** 2)[..., None] * np.array([150, 160, 200]) * 0.35
	moon = np.clip(1.0 - (dm - 46) / 2.5, 0, 1)
	sky = sky * (1 - moon[..., None]) + moon[..., None] * np.array([232, 230, 215])
	# stars, fading towards the horizon
	img = Image.fromarray(np.clip(sky, 0, 255).astype(np.uint8), 'RGB').convert('RGBA')
	# stars go on their own overlay: drawing straight into the sky would replace its alpha
	stars = Image.new('RGBA', (W, H), (0, 0, 0, 0))
	draw = ImageDraw.Draw(stars)
	for _ in range(900):
		x = rng.uniform(0, W)
		y = rng.uniform(0, H * 0.55)
		fade = 1.0 - y / (H * 0.55)
		a = int(rng.uniform(40, 230) * fade)
		r = rng.choice([0.6, 0.8, 1.0, 1.4])
		if math.hypot(x - mx, y - my) < 56:
			continue
		draw.ellipse([x - r, y - r, x + r, y + r], fill=(255, 250, 235, a))
	img.alpha_composite(stars)
	return img


def layer_mountains(rng):
	img = Image.new('RGBA', (W, H), (0, 0, 0, 0))
	draw = ImageDraw.Draw(img)
	far = ridge(rng, W, H * 0.60, 70, 6)
	fill_below(draw, far, (86, 70, 104, 255), H)
	near = ridge(rng, W, H * 0.66, 55, 6)
	fill_below(draw, near, (62, 50, 80, 255), H)
	# atmospheric haze: lighter towards the ridge line
	a = np.array(img).astype(np.float64)
	haze = vertical_gradient([(0.0, '#d08a70'), (0.55, '#d08a70'), (0.75, '#3e3250'), (1.0, '#2a2238')], W, H)
	t = np.clip((np.arange(H) - H * 0.5) / (H * 0.3), 0, 1)[:, None, None]
	a[..., :3] = a[..., :3] * t + haze * (1 - t) * 0.6 + a[..., :3] * (1 - t) * 0.4
	return Image.fromarray(np.clip(a, 0, 255).astype(np.uint8), 'RGBA')


def layer_forest(rng):
	img = Image.new('RGBA', (W, H), (0, 0, 0, 0))
	draw = ImageDraw.Draw(img)
	col = (38, 30, 48, 255)
	hills = ridge(rng, W, H * 0.73, 40, 5)
	fill_below(draw, hills, col, H)
	for x, y in hills[::2]:
		if rng.random() < 0.85:
			pine(draw, x + rng.uniform(-3, 3), y + 4, rng.uniform(26, 58), col)
	# radio tower on a hill
	tx = W * 0.80
	ty = ground_y(hills, tx)
	th = 230
	draw.line([(tx - 18, ty), (tx, ty - th)], fill=col, width=4)
	draw.line([(tx + 18, ty), (tx, ty - th)], fill=col, width=4)
	for i in range(1, 9):
		yy = ty - th * i / 9
		half = 18 * (1 - i / 9)
		draw.line([(tx - half, yy), (tx + half, yy)], fill=col, width=2)
	draw.ellipse([tx - 4, ty - th - 10, tx + 4, ty - th - 2], fill=(255, 60, 50, 255))
	return img


def layer_fog(rng):
	a = np.zeros((H, W), dtype=np.float64)
	yy, xx = np.mgrid[0:H, 0:W].astype(np.float64)
	for _ in range(46):
		cx = rng.uniform(-100, W + 100)
		cy = rng.uniform(H * 0.66, H * 0.86)
		rx = rng.uniform(160, 460)
		ry = rng.uniform(22, 60)
		a += np.exp(-(((xx - cx) / rx) ** 2 + ((yy - cy) / ry) ** 2)) * rng.uniform(0.25, 0.6)
	a = np.clip(a, 0, 1) * 150
	rgb = np.zeros((H, W, 3)) + np.array([196, 170, 200])
	out = np.dstack([rgb, a]).astype(np.uint8)
	return Image.fromarray(out, 'RGBA').filter(ImageFilter.GaussianBlur(6))


def house(draw, x, ground, w, h, roof, col, windows, rng, ruined):
	draw.rectangle([x, ground - h, x + w, ground + 4], fill=col)
	if ruined:
		draw.polygon([(x - 4, ground - h), (x + w * 0.35, ground - h - roof), (x + w * 0.55, ground - h - roof * 0.45),
					  (x + w * 0.62, ground - h - roof * 0.8), (x + w + 4, ground - h)], fill=col)
	else:
		draw.polygon([(x - 6, ground - h), (x + w / 2, ground - h - roof), (x + w + 6, ground - h)], fill=col)
	for _ in range(windows):
		wx = x + rng.uniform(6, w - 14)
		wy = ground - h + rng.uniform(8, h - 20)
		if rng.random() < 0.55:
			draw.rectangle([wx, wy, wx + 7, wy + 9], fill=(255, 176, 84, 255))


def layer_ruins(rng):
	img = Image.new('RGBA', (W, H), (0, 0, 0, 0))
	draw = ImageDraw.Draw(img)
	col = (22, 17, 28, 255)
	ground = ridge(rng, W, H * 0.82, 14, 3)
	fill_below(draw, ground, col, H)

	def gy(x):
		return ground_y(ground, x)

	x = 120.0
	while x < W - 160:
		w = rng.uniform(60, 120)
		h = rng.uniform(40, 80)
		house(draw, x, gy(x + w / 2), w, h, rng.uniform(22, 40), col, rng.randint(0, 3), rng, rng.random() < 0.35)
		x += w + rng.uniform(40, 170)
	# church with steeple
	cx = W * 0.42
	g = gy(cx)
	draw.rectangle([cx, g - 90, cx + 150, g + 4], fill=col)
	draw.polygon([(cx - 8, g - 90), (cx + 75, g - 140), (cx + 158, g - 90)], fill=col)
	draw.rectangle([cx + 150, g - 170, cx + 190, g + 4], fill=col)
	draw.polygon([(cx + 146, g - 170), (cx + 170, g - 260), (cx + 194, g - 170)], fill=col)
	draw.line([(cx + 170, g - 260), (cx + 170, g - 290)], fill=col, width=3)
	draw.line([(cx + 160, g - 280), (cx + 180, g - 280)], fill=col, width=3)
	draw.rectangle([cx + 164, g - 150, cx + 176, g - 130], fill=(255, 190, 110, 255))
	# power line poles with sagging cables
	poles = [W * 0.06 + i * 290 for i in range(7)]
	tops = []
	for px in poles:
		g = gy(px)
		draw.line([(px, g), (px, g - 120)], fill=col, width=4)
		draw.line([(px - 18, g - 112), (px + 18, g - 112)], fill=col, width=3)
		tops.append((px, g - 112))
	for (x1, y1), (x2, y2) in zip(tops, tops[1:]):
		pts = []
		for i in range(21):
			t = i / 20
			pts.append((x1 + (x2 - x1) * t, y1 + (y2 - y1) * t + math.sin(t * math.pi) * 26))
		draw.line(pts, fill=col, width=2)
	return img


def layer_foreground(rng):
	img = Image.new('RGBA', (W, H), (0, 0, 0, 0))
	draw = ImageDraw.Draw(img)
	col = (8, 6, 11, 255)
	ground = ridge(rng, W, H * 0.93, 18, 3)
	fill_below(draw, ground, col, H)
	# big framing pines on both sides
	for x, h in ((70, 900), (230, 640), (W - 120, 960), (W - 300, 700), (W - 30, 560)):
		pine(draw, x, H + 10, h, col)
	# grass blades
	for _ in range(2600):
		x = rng.uniform(0, W)
		base = ground_y(ground, x) + 6
		h = rng.uniform(10, 48)
		lean = rng.uniform(-14, 14)
		draw.line([(x, base), (x + lean, base - h)], fill=col, width=2)
	return img


def layer_vignette():
	w, h = 1024, 512
	yy, xx = np.mgrid[0:h, 0:w].astype(np.float64)
	d = np.sqrt(((xx - w / 2) / (w / 2)) ** 2 + ((yy - h / 2) / (h / 2)) ** 2)
	a = np.clip((d - 0.55) / 0.75, 0, 1) ** 1.6 * 235
	out = np.dstack([np.zeros((h, w, 3)), a]).astype(np.uint8)
	return Image.fromarray(out, 'RGBA')


def radial(size, color, power, peak):
	yy, xx = np.mgrid[0:size, 0:size].astype(np.float64)
	c = (size - 1) / 2
	d = np.sqrt((xx - c) ** 2 + (yy - c) ** 2) / c
	a = np.clip(1 - d, 0, 1) ** power * peak
	rgb = np.zeros((size, size, 3)) + np.array(color)
	return Image.fromarray(np.dstack([rgb, a]).astype(np.uint8), 'RGBA')


def main():
	if len(sys.argv) == 4 and sys.argv[1] == '--convert':
		encode_edds(Image.open(sys.argv[2]), sys.argv[3])
		print('wrote', sys.argv[3])
		return
	os.makedirs(OUT_DIR, exist_ok=True)
	os.makedirs(PREVIEW_DIR, exist_ok=True)
	print('Generating layers ->', OUT_DIR)
	builders = [
		('psy_ils_layer0_sky', layer_sky),
		('psy_ils_layer1_mountains', layer_mountains),
		('psy_ils_layer2_forest', layer_forest),
		('psy_ils_layer3_fog', layer_fog),
		('psy_ils_layer4_ruins', layer_ruins),
		('psy_ils_layer5_foreground', layer_foreground),
	]
	layers = []
	for i, (name, fn) in enumerate(builders):
		img = fn(random.Random(SEED + i))
		save(img, name)
		layers.append(img)
	save(layer_vignette(), 'psy_ils_vignette')
	save(radial(64, (255, 236, 200), 1.8, 255), 'psy_ils_particle')
	save(radial(512, (255, 214, 160), 2.2, 200), 'psy_ils_cursorlight')
	# flat composite for the docs
	comp = Image.new('RGBA', (W, H), (0, 0, 0, 255))
	for img in layers:
		comp.alpha_composite(img)
	comp.alpha_composite(layer_vignette().resize((W, H), Image.LANCZOS))
	comp.convert('RGB').save(os.path.join(PREVIEW_DIR, 'psy_ils_composite.png'))
	print('Preview composite written.')


if __name__ == '__main__':
	main()
