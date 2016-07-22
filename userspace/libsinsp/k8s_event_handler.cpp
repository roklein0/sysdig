//
// k8s_event_handler.cpp
//

#include "k8s_event_handler.h"
#include "sinsp.h"
#include "sinsp_int.h"

// filters normalize state and event JSONs, so they can be processed generically:
// event is turned into a single-node array, state is turned into ADDED event

std::string k8s_event_handler::EVENT_FILTER =
	"{"
	" type: .type,"
	" apiVersion: .object.apiVersion,"
	" kind: .object.kind,"
	" items:"
	" ["
	"  .object |"
	"  {"
	"   name: .metadata.name,"
	"   uid: .metadata.uid,"
	"   timestamp: .metadata.creationTimestamp,"
	"   reason: .reason,"
	"   message: .message,"
	"   involvedObject: .involvedObject"
	"  }"
	" ]"
	"}";

std::string k8s_event_handler::STATE_FILTER =
	"{"
	" type: \"ADDED\","
	" apiVersion: .apiVersion,"
	" kind: \"Event\","
	" items:"
	" ["
	"  .items[] |"
	"  {"
	"   name: .metadata.name,"
	"   uid: .metadata.uid,"
	"   timestamp: .metadata.creationTimestamp,"
	"   reason: .reason,"
	"   message: .message,"
	"   involvedObject: .involvedObject"
	"  }"
	" ]"
	"}";

k8s_event_handler::k8s_event_handler(k8s_state_t& state,
	collector_t& collector,
	std::string url,
	const std::string& http_version,
	ssl_ptr_t ssl,
	bt_ptr_t bt,
	filter_ptr_t event_filter):
		k8s_handler(collector, "k8s_event_handler", url, "/api/v1/events",
					STATE_FILTER, EVENT_FILTER, http_version,
					1000L, ssl, bt, &state),
		m_event_filter(event_filter)
{
}

k8s_event_handler::~k8s_event_handler()
{
}

void k8s_event_handler::handle_component(const Json::Value& json, const msg_data* data)
{
	if(m_event_filter)
	{
		if(m_state)
		{
			if(data)
			{
				if(!json.isNull())
				{
					g_logger.log("K8s EVENT: json found.", sinsp_logger::SEV_TRACE);
					const Json::Value& involved_json = json["involvedJson"];
					if(!involved_json.isNull())
					{
						bool is_aggregate = (get_json_string(json , "message").find("events with common reason combined") != std::string::npos);
						time_t last_ts = get_epoch_utc_seconds(get_json_string(json , "lastTimestamp"));
						time_t now_ts = get_epoch_utc_seconds_now();
						g_logger.log("K8s EVENT: lastTimestamp=" + std::to_string(last_ts) + ", now_ts=" + std::to_string(now_ts), sinsp_logger::SEV_TRACE);
						if(((last_ts > 0) && (now_ts > 0)) && // we got good timestamps
							!is_aggregate && // not an aggregated cached event
							((now_ts - last_ts) < 10)) // event not older than 10 seconds
						{
							const Json::Value& kind = involved_json["kind"];
							const Json::Value& event_reason = json["reason"];
							g_logger.log("K8s EVENT: involved json and event reason found:" + kind.asString() + '/' + event_reason.asString(), sinsp_logger::SEV_TRACE);
							if(!kind.isNull() && kind.isConvertibleTo(Json::stringValue) &&
								!event_reason.isNull() && event_reason.isConvertibleTo(Json::stringValue))
							{
								bool is_allowed = m_event_filter->allows_all();
								std::string type = kind.asString();
								if(!is_allowed && !type.empty())
								{
									std::string reason = event_reason.asString();
									is_allowed = m_event_filter->allows_all(type);
									if(!is_allowed && !reason.empty())
									{
										is_allowed = m_event_filter->has(type, reason);
									}
								}
								if(is_allowed)
								{
									g_logger.log("K8s EVENT: adding event.", sinsp_logger::SEV_TRACE);
									k8s_event_t& evt = m_state->add_component<k8s_events, k8s_event_t>(m_state->get_events(),
																data->m_name, data->m_uid, data->m_namespace);
									m_state->update_event(evt, json);
								}
								else
								{
									g_logger.log("K8s EVENT: filter does not allow {\"" + type + "\", \"{" + event_reason.asString() + "\"} }", sinsp_logger::SEV_TRACE);
									g_logger.log(m_event_filter->to_string(), sinsp_logger::SEV_TRACE);
								}
							}
							else
							{
								g_logger.log("K8s EVENT: event type or involvedJson kind not found.", sinsp_logger::SEV_ERROR);
								g_logger.log(Json::FastWriter().write(json), sinsp_logger::SEV_TRACE);
							}
						}
						else
						{
							g_logger.log("K8s EVENT: old event, ignoring: "
										 ", lastTimestamp=" + std::to_string(last_ts) + ", now_ts=" + std::to_string(now_ts),
										sinsp_logger::SEV_DEBUG);
						}
					}
					else
					{
						g_logger.log("K8s EVENT: involvedJson not found (null).", sinsp_logger::SEV_ERROR);
						g_logger.log(Json::FastWriter().write(json), sinsp_logger::SEV_TRACE);
					}
				}
				else
				{
					g_logger.log("K8s EVENT: json not found (null).", sinsp_logger::SEV_ERROR);
					g_logger.log(Json::FastWriter().write(json), sinsp_logger::SEV_TRACE);
				}
			}
			else
			{
				g_logger.log("K8s EVENT: msg data is null.", sinsp_logger::SEV_ERROR);
			}
		}
		else
		{
			g_logger.log("K8s EVENT: state is null.", sinsp_logger::SEV_ERROR);
		}
	}
	else
	{
		g_logger.log("K8s EVENT: filter NOT found.", sinsp_logger::SEV_DEBUG);
	}
}

void k8s_event_handler::handle_json(Json::Value&& root)
{
	if(g_logger.get_severity() >= sinsp_logger::SEV_TRACE)
	{
		g_logger.log(json_as_string(root), sinsp_logger::SEV_TRACE);
	}

	handle_component(root);
}
