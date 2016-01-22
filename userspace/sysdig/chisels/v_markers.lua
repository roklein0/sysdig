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
	id = "markers",
	name = "00Markers",
	description = "Show the top system calls in the system based on number of invocations and time spent calling them.",
	tags = {"Default"},
	view_type = "table",
	applies_to = {"", "container.id", "proc.pid", "proc.name", "thread.tid", "fd.directory", "evt.res", "k8s.pod.id", "k8s.rc.id", "k8s.svc.id", "k8s.ns.id"},
--	use_defaults = true,
--	filter = "evt.type=marker",
	filter = "marker.ntags>=%depth",
	drilldown_target = "markers",
	spectro_type = "markers",
	drilldown_increase_depth = true,
	columns = 
	{
		{
			name = "NA",
			field = "marker.tag[%depth]",
			is_key = true
		},
		{
			is_sorting = true,
			name = "#HITS",
			field = "marker.count.fortag[%depth]",
			description = "Number of calls per second for this system call.",
			colsize = 10,
			aggregation = "SUM"
		},
		{
			name = "AVG TIME",
			field = "marker.latency",
			description = "XXX",
			colsize = 10,
			aggregation = "AVG"
		},
		{
			name = "MIN TIME",
			field = "marker.latency",
			description = "XXX",
			colsize = 10,
			aggregation = "MIN"
		},
		{
			name = "MAX TIME",
			field = "marker.latency",
			description = "XXX",
			colsize = 10,
			aggregation = "MAX"
		},
		{
			name = "TAG",
			field = "marker.tag[%depth]",
			description = "Marker tag.",
			colsize = 32,
			aggregation = "SUM"
		},
	}
}
