import gobject, gtk
import ario

class SamplePython(ario.Plugin):

	def __init__(self):
		ario.Plugin.__init__(self)

	def activate(self, shell):
		print "activating sample python plugin"
                print ario.ario_mpd_get_current_artist()
	
	def deactivate(self, shell):
		print "deactivating sample python plugin"

