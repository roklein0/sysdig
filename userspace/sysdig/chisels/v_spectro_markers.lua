--[[
Copyright (C) 2013-2015 Draios inc.
 
This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License version 2 as
published by the Free Software Foundation.


This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.
--]]
view_info = 
{
	id = "spectro_app",
	name = "Markers Spectrogram",
	description = "Application markers latency spectrogram.",
	view_type = "spectrogram",
	applies_to = {"", "container.id", "proc.pid", "thread.tid", "proc.name", "evt.res", "k8s.pod.id", "k8s.rc.id", "k8s.svc.id", "k8s.ns.id", "fd.name", "fd.containername", "fd.directory", "fd.containerdirectory"},
	filter = "marker.ntags=%depth+1",
	use_defaults = false,
	drilldown_target = "marker_ids",
	propagate_filter = false,
	columns = 
	{
		{
			name = "NA",
			field = "marker.latency.quantized",
			is_key = true
		},
		{
			name = "LATENCY",
			description = "system call latency.",
			field = "marker.latency.quantized",
		},
		{
			name = "COUNT",
			description = "Number of markers.",
			field = "evt.count",
			aggregation = "SUM",
			colsize = 8,
		}
	}
}
