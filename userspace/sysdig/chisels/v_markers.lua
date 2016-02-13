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
	name = "Markers",
	description = "Show a summary of the application markers executing on the system. For each marker tag, the view reports information like how many times it's been called and how long it took to complete.",
	tips = {
		"Markers are sysdig's super easy way to delimit portions of your code so that sysdig can measure how long they take and tell you what's happening in them. You can learn about markers at XXX.",
		"For makers with hierarchical tags (e.g. 'api.loginrequest.processing'), only one level in the hierarch is shown (e.g. 'api'). Drilling down allows you to explore the next level.",
		"This view collapses multiple calls to a tag into a single line. If you instead want to see each single call, use the 'Markers List' view.",
	},
	tags = {"Default"},
	view_type = "table",
	applies_to = {"", "marker.tag", "marker.id", "container.id", "proc.pid", "proc.name", "thread.tid", "fd.directory", "evt.res", "k8s.pod.id", "k8s.rc.id", "k8s.svc.id", "k8s.ns.id"},
	use_defaults = true,
	filter = "marker.ntags>=%depth+1",
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
			description = "number of times the marker with the given tag has been hit.",
			colsize = 10,
			aggregation = "SUM"
		},
		{
			name = "AVG TIME",
			field = "marker.latency.fortag[%depth]",
			description = "the average time this marker took to complete.",
			colsize = 10,
			aggregation = "AVG"
		},
		{
			name = "MIN TIME",
			field = "marker.latency.fortag[%depth]",
			description = "the minimum time this marker took to complete.",
			colsize = 10,
			aggregation = "MIN"
		},
		{
			name = "MAX TIME",
			field = "marker.latency.fortag[%depth]",
			description = "the maximum time this marker took to complete.",
			colsize = 10,
			aggregation = "MAX"
		},
		{
			name = "TAG",
			field = "marker.tag[%depth]",
			description = "marker tag.",
			colsize = 256,
			aggregation = "SUM"
		},
	}
}
