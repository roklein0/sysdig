//
// k8s_node_handler.cpp
//

#include "k8s_node_handler.h"
#include "sinsp.h"
#include "sinsp_int.h"

// filters normalize state and event JSONs, so they can be processed generically:
// event is turned into a single-node array, state is turned into ADDED event

std::string k8s_node_handler::EVENT_FILTER =
	"{"
	" type: .type,"
	" apiVersion: .object.apiVersion,"
	" kind: .object.kind,"
	" nodes:"
	" ["
	"  .object |"
	"  {"
	"   name: .metadata.name,"
	"   uid: .metadata.uid,"
	"   timestamp: .metadata.creationTimestamp,"
	"   labels: .metadata.labels,"
	"   addresses: [.status.addresses[].address] | unique"
	"  }"
	" ]"
	"}";

std::string k8s_node_handler::STATE_FILTER =
	"{"
	" type: \"ADDED\","
	" apiVersion: .apiVersion,"
	" kind: \"Node\", "
	" nodes:"
	" ["
	"  .items[] | "
	"  {"
	"   name: .metadata.name,"
	"   uid: .metadata.uid,"
	"   timestamp: .metadata.creationTimestamp,"
	"   labels: .metadata.labels,"
	"   addresses: [.status.addresses[].address] | unique"
	"   }"
	" ]"
	"}";

k8s_node_handler::k8s_node_handler(k8s_state_t& state,
	std::string url,
	const std::string& http_version,
	ssl_ptr_t ssl,
	bt_ptr_t bt):
		k8s_handler("k8s_node_handler", url, "/api/v1/nodes",
					STATE_FILTER, EVENT_FILTER, http_version,
					1000L, ssl, bt),
		m_state(state)
{
}

k8s_node_handler::~k8s_node_handler()
{
}

void k8s_node_handler::handle_node(const msg_data& data, const Json::Value& n)
{
	k8s_node_t& node = m_state.get_component<k8s_nodes, k8s_node_t>(m_state.get_nodes(), data.m_name, data.m_uid);
	k8s_node_t::host_ip_list addresses = k8s_node_t::extract_addresses(n["addresses"]);
	if(addresses.size() > 0)
	{
		node.set_host_ips(std::move(addresses));
	}
	k8s_pair_list entries = k8s_component::extract_object(n, "labels");
	if(entries.size() > 0)
	{
		node.set_labels(std::move(entries));
	}
}

void k8s_node_handler::handle_json(Json::Value&& root)
{
	if(g_logger.get_severity() >= sinsp_logger::SEV_TRACE)
	{
		g_logger.log("********************************************************************", sinsp_logger::SEV_TRACE);
		g_logger.log(json_as_string(root), sinsp_logger::SEV_TRACE);
		g_logger.log("********************************************************************", sinsp_logger::SEV_TRACE);
	}

	const Json::Value& type = root["type"];
	if(!type.isNull())
	{
		if(type.isConvertibleTo(Json::stringValue))
		{
			const Json::Value& kind = root["kind"];
			if(!kind.isNull())
			{
				if(kind.isConvertibleTo(Json::stringValue))
				{
					std::string t = type.asString();
					std::string k = kind.asString();
					for(const Json::Value& n : root["nodes"])
					{
						msg_data data = get_msg_data(t, k, n);
						log_event(data);
						if(data.m_reason == COMPONENT_ADDED)
						{
							if(m_state.has(m_state.get_nodes(), data.m_uid))
							{
								std::ostringstream os;
								os << "K8s ADDED message received for existing node [" << data.m_uid << "], updating only.";
								g_logger.log(os.str(), sinsp_logger::SEV_DEBUG);
							}
							handle_node(data, n);
						}
						else if(data.m_reason == COMPONENT_MODIFIED)
						{
							if(!m_state.has(m_state.get_nodes(), data.m_uid))
							{
								std::ostringstream os;
								os << "K8s MODIFIED message received for non-existing node [" << data.m_uid << "], giving up.";
								g_logger.log(os.str(), sinsp_logger::SEV_ERROR);
								return;
							}
							handle_node(data, n);
						}
						else if(data.m_reason == COMPONENT_DELETED)
						{
							if(!m_state.delete_component(m_state.get_nodes(), data.m_uid))
							{
								g_logger.log(std::string("K8s NODE not found: ") + data.m_name, sinsp_logger::SEV_ERROR);
							}
						}
						else if(data.m_reason == COMPONENT_ERROR)
						{
							log_error(root, "NODE");
						}
						else
						{
							g_logger.log(std::string("Unsupported K8S NODE event reason: ") + std::to_string(data.m_reason),
										 sinsp_logger::SEV_ERROR);
						}
					} // end for nodes
				}
			}
		}
		else
		{
			g_logger.log(std::string("K8S NODE event type is not string."), sinsp_logger::SEV_ERROR);
		}
	}
	else
	{
		g_logger.log(std::string("K8S NODE event type is null."), sinsp_logger::SEV_ERROR);
	}
}

void k8s_node_handler::log_error(const Json::Value& root, const std::string& comp)
{
	std::string unk_err = "Unknown.";
	std::ostringstream os;
	os << "K8S server reported " << comp << " error: ";
	if(!root.isNull())
	{
		Json::Value object = root["object"];
		if(!object.isNull())
		{
			os << object.toStyledString();
			unk_err.clear();
		}
	}
	os << unk_err;
	g_logger.log(os.str(), sinsp_logger::SEV_ERROR);
}
