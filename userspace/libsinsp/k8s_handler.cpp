//
// k8s_handler.cpp
//

#include "k8s_handler.h"
#include "sinsp.h"
#include "sinsp_int.h"

k8s_handler::k8s_handler(collector_t& collector,
	const std::string& id,
	std::string url,
	const std::string& path,
	const std::string& state_filter,
	const std::string& event_filter,
	const std::string& http_version,
	int timeout_ms,
	ssl_ptr_t ssl,
	bt_ptr_t bt,
	k8s_state_t* state): m_state(state),
		m_collector(collector),
		m_id(id + "_state"),
		m_path(path),
		m_state_filter(state_filter),
		m_event_filter(event_filter),
		m_filter(m_state_filter),
		m_timeout_ms(timeout_ms),
		m_url(url),
		m_http_version(http_version),
		m_ssl(ssl),
		m_bt(bt)
{
	g_logger.log(std::string("Creating K8s handler object for " + m_id + " (" + m_url + ")"),
				 sinsp_logger::SEV_DEBUG);
	make_http();
	connect(1);
	g_logger.log(std::string("Sent K8s handler connect request for " + m_id + " (" + m_url + ")"),
				 sinsp_logger::SEV_DEBUG);
}

k8s_handler::~k8s_handler()
{
}

void k8s_handler::make_http()
{
	m_http = std::make_shared<handler_t>(*this, m_id,
								 m_url, m_path, m_http_version,
								 m_timeout_ms, m_ssl, m_bt);
	m_http->set_json_callback(&k8s_handler::set_event_json);
	m_http->set_json_end("}\n");
	m_http->set_json_filter(m_filter);
	m_collector.add(m_http);
}

bool k8s_handler::connect(int expected_connections)
{
	g_logger.log(std::string("k8s_handler (" + m_id + ") connect begin for " + m_url),
				 sinsp_logger::SEV_TRACE);
	if(m_http)
	{
		if(!m_http->is_connecting())
		{
			g_logger.log(std::string("k8s_handler (" + m_id +
									 ") connect is not connecting " + m_url), sinsp_logger::SEV_TRACE);
			if(m_collector.has(m_http))
			{
				g_logger.log(std::string("k8s_handler (" + m_id + ") collector has " + m_url),
							 sinsp_logger::SEV_TRACE);
				if(!m_http->is_connected())
				{
					g_logger.log(std::string("k8s_handler (" + m_id +
											 ") http is not connected to " + m_url), sinsp_logger::SEV_TRACE);
					m_collector.remove(m_http);
					m_http.reset();
				}
			}
			if(!m_collector.has(m_http))
			{
				g_logger.log(std::string("k8s_handler (" + m_id +
										 ") collector hasn't " + m_url), sinsp_logger::SEV_TRACE);
				make_http();
			}
			g_logger.log(std::string("k8s_handler (" + m_id +
									 ") collector checking status ... " + m_url), sinsp_logger::SEV_TRACE);
			check_collector_status(expected_connections);
			bool has_http = m_collector.has(m_http);
			g_logger.log(std::string("k8s_handler (" + m_id +
									 ") collector has " + (has_http ? std::string() : "not ") + m_url), sinsp_logger::SEV_TRACE);
			return has_http;
		}
		else
		{
			g_logger.log(std::string("k8s_handler (" + m_id +
									 ") connect, http connecting to " + m_url), sinsp_logger::SEV_TRACE);
		}
	}
	else
	{
		g_logger.log(std::string("k8s_handler (" + m_id +
									 ") connect, http is null"), sinsp_logger::SEV_WARNING);
	}
	return false;
}

void k8s_handler::send_data_request()
{
	if(m_http && !m_req_sent)
	{
		if(m_http->is_connected())
		{
			m_http->send_request();
			m_req_sent = true;
		}
		else if(m_http->is_connecting())
		{
			g_logger.log("k8s_handler (" + m_id + ") is conecting to " + m_url,
						 sinsp_logger::SEV_DEBUG);
		}
	}
	else
	{
		throw sinsp_exception("k8s_handler (" + m_id + ") HTTP client (" + m_url + ") is null.");
	}
}

bool k8s_handler::is_alive() const
{
	if(m_http && !m_http->is_connecting() && !m_http->is_connected())
	{
		g_logger.log("k8s_handler (" + m_id + ") state connection (" + m_url + ") loss.",
					 sinsp_logger::SEV_WARNING);
		return false;
	}
	return true;
}

void k8s_handler::check_collector_status(int expected)
{
	if(!m_collector.is_healthy(expected))
	{
		throw sinsp_exception("k8s_handler (" + m_id + ") collector not healthy (has " +
							  std::to_string(m_collector.subscription_count()) +
							  " connections, expected " + std::to_string(expected) +
							  "); giving up on data collection in this cycle ...");
	}
}

void k8s_handler::collect_data()
{
	if(m_http)
	{
		if(m_http->is_connecting())
		{
			g_logger.log("k8s_handler (" + m_id + ") is connecting to " + m_url, sinsp_logger::SEV_DEBUG);
			return;
		}
		else if(m_http->is_connected())
		{
			if(!m_req_sent)
			{
				g_logger.log("k8s_handler (" + m_id + ") connected to " + m_url, sinsp_logger::SEV_DEBUG);
				send_data_request();
			}
			if(m_collector.subscription_count())
			{
				m_collector.get_data();
				if(m_events.size())
				{
					for(auto evt : m_events)
					{
						if(evt && !evt->isNull())
						{
							handle_json(std::move(*evt));
						}
						else
						{
							g_logger.log("k8s_handler (" + m_id + ") error (" + m_url + ") " +
										(!evt ? "data is null." : (evt->isNull() ? "JSON is null." : "Unknown")),
										sinsp_logger::SEV_ERROR);
						}
					}
					m_events.clear();
					if(!m_state_built)
					{
						// done with initial state handling, switch to events
						m_state_built = true;
						m_collector.remove(m_http);
						m_http.reset();
						std::string::size_type pos = m_id.find("_state");
						if(pos != std::string::npos)
						{
							m_id = m_id.substr(0, pos).append("_event");
						}
						pos = m_path.find("/watch");
						if(pos == std::string::npos)
						{
							pos = m_path.rfind('/');
							if(pos != std::string::npos)
							{
								m_path.insert(pos, "/watch");
							}
							else
							{
								throw sinsp_exception("k8s_handler (" + m_id + "), invalid URL path: " + m_url);
							}
						}
						m_filter = m_event_filter;
						m_req_sent = false;
						make_http();
						connect(1);
					}
				}
			}
			return;
		}
		m_req_sent = false;
		throw sinsp_exception("k8s_handler (" + m_id + "), m_http interface not connected to " + m_url);
	}
	else
	{
		throw sinsp_exception("k8s_handler (" + m_id + "), no m_http interface found (null) for " + m_url + ").");
	}
}

k8s_handler::msg_data k8s_handler::get_msg_data(const std::string& type, const std::string& kind, const Json::Value& json)
{
	msg_data data;
	if(!type.empty())
	{
		if(type[0] == 'A') { data.m_reason = COMPONENT_ADDED; }
		else if(type[0] == 'M') { data.m_reason = COMPONENT_MODIFIED; }
		else if(type[0] == 'D') { data.m_reason = COMPONENT_DELETED; }
		else if(type[0] == 'E') { data.m_reason = COMPONENT_ERROR; }
	}
	else
	{
		return data;
	}

	data.m_kind = kind;

	Json::Value name = json["name"];
	if(!name.isNull())
	{
		data.m_name = name.asString();
	}
	Json::Value uid = json["uid"];
	if(!uid.isNull())
	{
		data.m_uid = uid.asString();
	}
	Json::Value nspace = json["namespace"];
	if(!nspace.isNull())
	{
		data.m_namespace = nspace.asString();
	}

	return data;
}

void k8s_handler::handle_json(Json::Value&& root)
{
	if(g_logger.get_severity() >= sinsp_logger::SEV_TRACE)
	{
		g_logger.log("********************************************************************", sinsp_logger::SEV_TRACE);
		g_logger.log(json_as_string(root), sinsp_logger::SEV_TRACE);
		g_logger.log("********************************************************************", sinsp_logger::SEV_TRACE);
	}

	if(!m_state)
	{
		throw sinsp_exception("k8s_handler (" + m_id + "), state is null for " + m_url + ").");
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
					for(const Json::Value& item : root["items"])
					{
						msg_data data = get_msg_data(t, k, item);
						log_event(data);
						if(data.m_reason == COMPONENT_ADDED)
						{
							if(m_state->has(data.m_uid))
							{
								std::ostringstream os;
								os << "K8s ADDED message received for existing " << data.m_kind <<
									" [" << data.m_uid << "], updating only.";
								g_logger.log(os.str(), sinsp_logger::SEV_DEBUG);
							}
							handle_component(item, &data);
						}
						else if(data.m_reason == COMPONENT_MODIFIED)
						{
							if(!m_state->has(data.m_uid))
							{
								std::ostringstream os;
								os << "K8s MODIFIED message received for non-existing " << data.m_kind <<
									" [" << data.m_uid << "], giving up.";
								g_logger.log(os.str(), sinsp_logger::SEV_ERROR);
								return;
							}
							handle_component(item, &data);
						}
						else if(data.m_reason == COMPONENT_DELETED)
						{
							if(!m_state->delete_component(m_state->get_nodes(), data.m_uid))
							{
								g_logger.log(std::string("K8s " + data.m_kind + " not found: ") + data.m_name,
											 sinsp_logger::SEV_ERROR);
							}
						}
						else if(data.m_reason == COMPONENT_ERROR)
						{
							log_error(root, data.m_kind);
						}
						else
						{
							g_logger.log(std::string("Unsupported K8S " + data.m_kind + " event reason: ") +
										 std::to_string(data.m_reason), sinsp_logger::SEV_ERROR);
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

bool k8s_handler::is_ip_address(const std::string& addr)
{
	struct sockaddr_in serv_addr = {0};
	return inet_aton(addr.c_str(), &serv_addr.sin_addr);
}

k8s_handler::ip_addr_list_t k8s_handler::hostname_to_ip(const std::string& hostname)
{
	ip_addr_list_t ip_addrs;
	struct addrinfo *servinfo = 0;

	struct addrinfo hints = {0};
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;

	if((getaddrinfo(hostname.c_str(), NULL, &hints, &servinfo)) != 0)
	{
		g_logger.log("Can't determine IP address for hostname: " + hostname, sinsp_logger::SEV_WARNING);
		return ip_addrs;
	}

	for(struct addrinfo* p = servinfo; p != NULL; p = p->ai_next)
	{
		struct sockaddr_in* h = (struct sockaddr_in*)p->ai_addr;
		ip_addrs.emplace(inet_ntoa(h->sin_addr));
	}

	freeaddrinfo(servinfo);
	return ip_addrs;
}

void k8s_handler::set_event_json(json_ptr_t json, const std::string&)
{
	if(json)
	{
		m_events.emplace_back(json);
	}
	else
	{
		g_logger.log("K8s: delegator (" + m_id + ") received null JSON", sinsp_logger::SEV_ERROR);
	}
}

k8s_pair_list k8s_handler::extract_object(const Json::Value& object)
{
	k8s_pair_list entry_list;
	if(!object.isNull() && object.isObject())
	{
		Json::Value::Members members = object.getMemberNames();
		for (auto& member : members)
		{
			const Json::Value& val = object[member];
			if(!val.isNull() && val.isString())
			{
				entry_list.emplace_back(k8s_pair_t(member, val.asString()));
			}
		}
	}
	return entry_list;
}


void k8s_handler::log_error(const Json::Value& root, const std::string& comp)
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
