"""
Psyerns Interactive Loadingscreen - "Deadmans Echo" preset builder.

Splits the three flat FixedLoadingScreen artworks (dme1/2/3) into parallax layers:

    bg      the full picture with the red lettering painted out, soft under ground/face
    ground  the dark lower part of the picture (feathered top edge)
    face    the silhouette band at the left (dme1, dme2) or right (dme3) edge
    text    "DEADMANS ECHO" + URL, cut out by colour, with a soft drop shadow

plus a dark smoke band shared by all three. Output: .edds into the mod's gui/textures,
PNG previews and an animated parallax preview GIF into Psyerns_Interactive_Loadingscreen_Doks/previews.

Usage:
    python -m pip install pillow lz4 numpy
    python psy_ils_build_dme_preset.py "C:/path/to/FixedLoadingScreen/data"
"""
import os
import sys

import numpy as np
from PIL import Image, ImageFilter

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import psy_ils_generate_textures as gen  # noqa: E402

OUT_W = 2048
OUT_H = 1024
SOURCES = {
	# name: side of the face silhouette band
	'dme1': 'left',
	'dme2': 'left',
	'dme3': 'right',
}

# lettering boxes in source coordinates (1456x816 artwork, same template in all three);
# tight on purpose: the red sky left of the title and the red face band right of it must stay out
TEXT_BOXES = [(366, 35, 1092, 148), (366, 535, 1092, 608)]


def to_np(img):
	return np.asarray(img.convert('RGBA')).astype(np.float64)


def to_img(a):
	return Image.fromarray(np.clip(a, 0, 255).astype(np.uint8), 'RGBA')


def blur_np(a, radius):
	"""Gaussian blur per channel of a float HxW or HxWxC array."""
	if a.ndim == 3:
		return np.dstack([blur_np(a[..., c], radius) for c in range(a.shape[2])])
	size = max(1, int(radius * 3))
	k = np.exp(-0.5 * (np.arange(-size, size + 1) / radius) ** 2)
	k /= k.sum()
	padded = np.pad(a, ((0, 0), (size, size)), mode='edge')
	rows = np.apply_along_axis(lambda r: np.convolve(r, k, mode='valid'), 1, padded)
	padded = np.pad(rows, ((size, size), (0, 0)), mode='edge')
	return np.apply_along_axis(lambda c: np.convolve(c, k, mode='valid'), 0, padded)


def fill_masked(rgb, mask, radius):
	"""Normalized-convolution inpaint: masked pixels get the blurred colour of their unmasked surroundings."""
	keep = 1.0 - mask
	num = blur_np(rgb * keep[..., None], radius)
	den = blur_np(keep, radius)[..., None]
	filled = num / np.maximum(den, 1e-4)
	return rgb * keep[..., None] + filled * mask[..., None]


def fill_vertical(rgb, mask):
	"""Fills every masked vertical run with mirrored pixels from above and below, cross-faded.
	The DME artworks are built from vertical strips, so this keeps strip edges and grain intact
	where a blur would leave a smooth smudge."""
	out = rgb.copy()
	h, w = mask.shape
	for x in range(w):
		col = mask[:, x]
		y = 0
		while y < h:
			if not col[y]:
				y += 1
				continue
			start = y
			while y < h and col[y]:
				y += 1
			end = y  # exclusive
			n = end - start
			for k in range(n):
				above = start - 1 - k
				below = end + (n - 1 - k)
				if above < 0:
					above = min(h - 1, below)
				if below >= h:
					below = max(0, above)
				t = (k + 0.5) / n
				out[start + k, x] = rgb[above, x] * (1.0 - t) + rgb[below, x] * t
	return out


def text_mask(rgb):
	h, w, _ = rgb.shape
	r = rgb[..., 0]
	g = rgb[..., 1]
	b = rgb[..., 2]
	red = (r > 170) & (g < 90) & (b < 90) & (r - g > 110)
	box = np.zeros((h, w), dtype=bool)
	for x0, y0, x1, y1 in TEXT_BOXES:
		box[y0:y1, x0:x1] = True
	return (red & box).astype(np.float64)


def ramp(values, start, end):
	"""0 at start, 1 at end (works in both directions)."""
	t = (values - start) / (end - start)
	return np.clip(t, 0.0, 1.0)


def build(src_dir, name, side):
	path = os.path.join(src_dir, name + '.edds')
	src = gen.decode_edds(path) if os.path.exists(path) else Image.open(os.path.join(src_dir, name + '.png'))
	a = to_np(src)
	h, w, _ = a.shape
	rgb = a[..., :3]

	# --- lettering -------------------------------------------------------------------------------
	core = text_mask(rgb)
	core_soft = blur_np(core, 0.8)
	text_alpha = np.clip(core_soft * 1.6, 0, 1)
	# the letters have a dark outline that the colour mask misses - paint out generously
	grown = np.clip(blur_np(core, 5.0) * 10.0, 0, 1)
	shadow = np.clip(blur_np(np.roll(np.roll(core, 4, axis=0), 3, axis=1), 6.0) * 2.2, 0, 1) * 0.75

	text_rgb = rgb * text_alpha[..., None]
	out_alpha = text_alpha + shadow * (1.0 - text_alpha)
	text = np.dstack([text_rgb / np.maximum(out_alpha[..., None], 1e-4), out_alpha * 255])

	# background without lettering
	# one solid band per text line (letter-shaped holes leave visible mirror edges)
	region = np.zeros((h, w), dtype=bool)
	for x0, y0, x1, y1 in TEXT_BOXES:
		ys_hit, xs_hit = np.nonzero(core[y0:y1, x0:x1])
		if len(ys_hit) == 0:
			continue
		region[y0 + ys_hit.min() - 8:y0 + ys_hit.max() + 9, x0 + xs_hit.min() - 8:x0 + xs_hit.max() + 9] = True
	filled = fill_vertical(rgb, region)
	edge = blur_np(region.astype(np.float64), 3.0)
	clean = rgb * (1.0 - edge[..., None]) + filled * edge[..., None]

	# --- face band ---------------------------------------------------------------------------------
	xs = np.arange(w, dtype=np.float64)[None, :].repeat(h, axis=0)
	if side == 'left':
		face_a = 1.0 - ramp(xs, w * 0.16, w * 0.235)
	else:
		face_a = ramp(xs, w * 0.765, w * 0.84)

	# --- ground --------------------------------------------------------------------------------------
	ys = np.arange(h, dtype=np.float64)[:, None].repeat(w, axis=1)
	ground_a = ramp(ys, h * 0.70, h * 0.82)

	# under the moving layers the background is softened, so their shifted copies never double up
	soft = blur_np(clean, 24) * 0.8
	under = np.clip(np.maximum(face_a, ground_a) * 1.15, 0, 1)
	bg_rgb = clean * (1.0 - under[..., None]) + soft * under[..., None]

	layers = {
		'bg': np.dstack([bg_rgb, np.full((h, w), 255.0)]),
		'ground': np.dstack([clean, ground_a * 255]),
		'face': np.dstack([clean, face_a * 255]),
		'text': text,
	}
	result = {}
	for key, arr in layers.items():
		img = to_img(arr).resize((OUT_W, OUT_H), Image.LANCZOS)
		result[key] = img
		gen.save(img, f'psy_ils_{name}_{key}')
	return result


def smoke(seed=7):
	rng = np.random.default_rng(seed)
	w, h = OUT_W, OUT_H
	yy, xx = np.mgrid[0:h, 0:w].astype(np.float64)
	a = np.zeros((h, w))
	for _ in range(40):
		cx = rng.uniform(-100, w + 100)
		cy = rng.uniform(h * 0.62, h * 0.95)
		rx = rng.uniform(180, 520)
		ry = rng.uniform(30, 80)
		a += np.exp(-(((xx - cx) / rx) ** 2 + ((yy - cy) / ry) ** 2)) * rng.uniform(0.25, 0.6)
	a = np.clip(a, 0, 1) * 170
	rgb = np.zeros((h, w, 3)) + np.array([34, 30, 32])
	img = Image.fromarray(np.dstack([rgb, a]).astype(np.uint8), 'RGBA').filter(ImageFilter.GaussianBlur(8))
	gen.save(img, 'psy_ils_dme_smoke')
	return img


def parallax_gif(layers, smoke_img, name, frames=28):
	"""Small animated preview of the parallax: the camera circles once."""
	pw, ph = 560, 315
	depth = {'bg': 0.06, 'ground': 0.5, 'face': 0.8, 'text': 0.3}
	shift = 0.04
	scaled = {k: v.resize((int(pw * 1.12), int(ph * 1.12)), Image.LANCZOS) for k, v in layers.items()}
	smoke_s = smoke_img.resize((int(pw * 1.12), int(ph * 1.12)), Image.LANCZOS)
	vignette = gen.layer_vignette().resize((pw, ph), Image.LANCZOS)
	out = []
	for i in range(frames):
		t = i / frames * 2 * np.pi
		cx = np.cos(t)
		cy = np.sin(t) * 0.6
		frame = Image.new('RGBA', (pw, ph), (0, 0, 0, 255))
		order = [('bg', scaled['bg']), ('smoke', smoke_s), ('ground', scaled['ground']), ('face', scaled['face']), ('text', scaled['text'])]
		for key, img in order:
			d = depth.get(key, 0.3)
			ox = int((pw - img.width) / 2 - cx * shift * pw * d)
			oy = int((ph - img.height) / 2 - cy * shift * 0.6 * ph * d)
			frame.alpha_composite(img, (ox, oy))
		frame.alpha_composite(vignette)
		out.append(frame.convert('RGB').convert('P', palette=Image.ADAPTIVE, colors=128))
	out[0].save(os.path.join(gen.PREVIEW_DIR, f'psy_ils_{name}_parallax.gif'), save_all=True, append_images=out[1:], duration=70, loop=0)


def main():
	if len(sys.argv) < 2:
		print(__doc__)
		sys.exit(1)
	src_dir = sys.argv[1]
	os.makedirs(gen.OUT_DIR, exist_ok=True)
	os.makedirs(gen.PREVIEW_DIR, exist_ok=True)
	print('Building Deadmans Echo preset ->', gen.OUT_DIR)
	smoke_img = smoke()
	for name, side in SOURCES.items():
		layers = build(src_dir, name, side)
		parallax_gif(layers, smoke_img, name)
	print('Done.')


if __name__ == '__main__':
	main()
