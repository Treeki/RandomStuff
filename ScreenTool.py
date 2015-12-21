#!/usr/bin/python

# Ninji's ScreenTool
# Quick and dirty utility for taking and munging screenshots
# Dependencies:
#   KolourPaint (for 'Edit' button)
#   sshd / scp (for uploading)
#   xclip (for uploading)
#   maim/slop (for choosing images)
#   ImageMagick (for converting to JPG)
#   Python 3, python-gobject
#   More I've forgotten about, maybe?

# Configuration stuff

# Shell script containing environment variables as generated by the 'keychain' util
KEYCHAIN_PATH = '/home/ninji/.keychain/vahran-sh'

# scp destination for uploaded images
IMAGE_PATH_PREFIX = 'arctic:/srv/img/'

# prepended to generated URLs
IMAGE_URL_PREFIX = 'https://img.wuffs.org/'




import subprocess
import tempfile
import os
import datetime
import base64
import sys

from gi.repository import GLib
from gi.repository import Gdk
from gi.repository import GdkPixbuf
from gi.repository import Gtk
from gi.repository import Notify

def read_keychain(path):
	for line in open(path, 'r'):
		line = line.strip().split('; ')[0]
		key,value = line.split('=', 1)
		os.putenv(key, value)

def generate_token():
	now = datetime.datetime.now()
	stamp = now.strftime('%y%m%d.%H%M%S.')
	key = base64.b32encode(open('/dev/urandom', 'rb').read(5)).decode('latin-1').lower()
	return stamp+key

def allocate_temp_image_file(extension):
	handle,path = tempfile.mkstemp('.'+extension)
	os.close(handle)
	return path

def exec_slop():
	try:
		return subprocess.check_output('slop -b 5 -p 0 -t 2 -g 0.4 -c 0.5,0.5,0.5,1 --min=0 --max=0 -f %g'.split(' '))
	except subprocess.CalledProcessError:
		return None

def exec_maim(path):
	geometry = exec_slop()
	if not geometry:
		return False

	subprocess.call(['/usr/bin/maim', '-g', geometry, path])
	return True

#def exec_scrot(path):
#	args = ['/usr/bin/scrot', '-b', '-s', path]
#	subprocess.call(args)


class ScreenToolWindow(Gtk.Window):
	def __init__(self, png_path):
		Gtk.Window.__init__(self, title='ScreenTool')

		self.edit_button = Gtk.Button(label='Edit')
		self.jpg_button = Gtk.Button(label='Convert to JPG')
		self.upload_button = Gtk.Button(label='Upload')
		self.copy_button = Gtk.Button(label='Copy')

		self.edit_button.connect('clicked', self.on_edit_button_clicked)
		self.jpg_button.connect('clicked', self.on_jpg_button_clicked)
		self.upload_button.connect('clicked', self.on_upload_button_clicked)
		self.copy_button.connect('clicked', self.on_copy_button_clicked)

		self.png_radio = Gtk.RadioButton.new_with_label(None, 'PNG')
		self.jpg_radio = Gtk.RadioButton.new_with_label_from_widget(self.png_radio, 'JPG')
		self.png_radio.connect('toggled', self.on_radio_toggled)
		self.jpg_radio.set_sensitive(False)

		self.size_label = Gtk.Label(label='___')

		self.previewer = Gtk.Image()
		self.previewer_wrapper = Gtk.ScrolledWindow()
		self.previewer_wrapper.set_size_request(500,400)
		self.previewer_wrapper.add(self.previewer)

		self.top_box = Gtk.Box(orientation=Gtk.Orientation.HORIZONTAL, spacing=6)
		self.top_box.pack_start(self.edit_button, False, False, 0)
		self.top_box.pack_start(self.jpg_button, False, False, 0)
		self.top_box.pack_start(self.upload_button, False, False, 0)
		self.top_box.pack_start(self.copy_button, False, False, 0)
		self.top_box.pack_start(self.png_radio, False, False, 0)
		self.top_box.pack_start(self.jpg_radio, False, False, 0)
		self.top_box.pack_start(self.size_label, False, False, 0)

		self.box = Gtk.Box(orientation=Gtk.Orientation.VERTICAL, spacing=6)
		self.box.pack_start(self.top_box, False, False, 0)
		self.box.pack_start(self.previewer_wrapper, True, True, 0)
		self.add(self.box)

		self.png_path = png_path
		self.jpg_path = None
		self.show_preview(png_path)

	def show_preview(self, path):
		self.current_preview = path

		self.previewer.set_from_file(path)

		size = os.stat(path).st_size
		self.size_label.set_label('%d KB' % (size / 1024))

	def reload_preview(self):
		self.show_preview(self.current_preview)

	def on_edit_button_clicked(self, widget):
		subprocess.call(['/usr/bin/kolourpaint', self.current_preview])
		self.reload_preview()

	def on_jpg_button_clicked(self, widget):
		if self.jpg_path is None:
			self.jpg_path = allocate_temp_image_file('jpg')
		subprocess.call(['/usr/bin/convert', self.png_path, '-quality', '91', self.jpg_path])
		self.jpg_radio.set_sensitive(True)

	def on_upload_button_clicked(self, widget):
		global upload_path
		if self.jpg_radio.get_active():
			upload_path = self.jpg_path
		else:
			upload_path = self.png_path
		self.close()

	def on_copy_button_clicked(self, widget):
		global stay_around
		pb = GdkPixbuf.Pixbuf.new_from_file(self.png_path)
		cb = Gtk.Clipboard.get(Gdk.SELECTION_CLIPBOARD)
		cb.set_image(pb)
		cb.store()
		stay_around = True
		self.close()

	def on_radio_toggled(self, widget):
		if self.jpg_radio.get_active():
			self.show_preview(self.jpg_path)
		else:
			self.show_preview(self.png_path)


def maybe_exit(a, b):
	if stay_around:
		GLib.timeout_add(1000*60, Gtk.main_quit)
	else:
		Gtk.main_quit()


png_path = allocate_temp_image_file('png')
upload_path, upload_ext = None, None
stay_around = False

print(png_path)

if not exec_maim(png_path):
	sys.exit()

token = generate_token()

win = ScreenToolWindow(png_path)
win.connect('delete-event', maybe_exit)
win.show_all()
Gtk.main()

if upload_path is not None:
	if KEYCHAIN_PATH is not None:
		read_keychain(KEYCHAIN_PATH)

	Notify.init('ScreenTool')
	n = Notify.Notification.new('Uploading...', '... ... ...', 'dialog-information')
	n.show()

	upload_ext = upload_path[upload_path.rfind('.')+1:]
	subprocess.call(['chmod', 'a+r', upload_path])
	subprocess.call(['scp', upload_path, '%s%s.%s' % (IMAGE_PATH_PREFIX, token, upload_ext)])

	url = '%s%s.%s' % (IMAGE_URL_PREFIX, token, upload_ext)

	p = subprocess.Popen(['/usr/bin/xclip', '-selection', 'clipboard'], stdin=subprocess.PIPE)
	p.stdin.write(url.encode('utf-8'))
	p.stdin.close()
	p.wait()

	n.update('Image uploaded', url, 'dialog-information')
	n.show()

