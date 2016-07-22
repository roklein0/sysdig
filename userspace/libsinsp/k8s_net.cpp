//
// k8s_net.cpp
//

#ifdef HAS_CAPTURE

#include "k8s_net.h"
#include "k8s_component.h"
#include "k8s_node_handler.h"
#include "k8s_namespace_handler.h"
#include "k8s_pod_handler.h"
#include "k8s_replicationcontroller_handler.h"
#include "k8s_replicaset_handler.h"
#include "k8s_service_handler.h"
#include "k8s_daemonset_handler.h"
#include "k8s_deployment_handler.h"
#include "k8s_event_handler.h"
#include "k8s.h"
#include "sinsp.h"
#include "sinsp_int.h"
#include <sstream>
#include <utility>
#include <memory>


k8s_net::k8s_net(k8s& kube, k8s_state_t& state, const std::string& uri,
	ssl_ptr_t ssl,
	bt_ptr_t bt,
	bool curl_debug,
	ext_list_ptr_t extensions,
	filter_ptr_t event_filter) : m_k8s(kube), m_state(state),
		m_uri(uri),
		m_ssl(ssl),
		m_bt(bt),
		m_stopped(true),
		//m_collector(kube.watch_in_thread()),
		m_curl_debug(curl_debug),
		m_extensions(extensions),
		m_event_filter(event_filter)
#ifndef K8S_DISABLE_THREAD
		,m_thread(0)
#endif
{
}

k8s_net::~k8s_net()
{
	end_thread();
	cleanup();
}

void k8s_net::cleanup()
{/*
	unsubscribe();
	for (auto& component : m_api_interfaces)
	{
		delete component.second;
	}
	m_api_interfaces.clear();*/
	stop_watching();
	m_handlers.clear();
}

void k8s_net::watch()
{/*
	bool in_thread = m_k8s.watch_in_thread();
#ifdef K8S_DISABLE_THREAD
	if(in_thread)
	{
		g_logger.log("Thread run requested for non-thread binary.", sinsp_logger::SEV_WARNING);
	}
#else
	if(m_stopped && in_thread)
	{
		subscribe();
		m_stopped = false;
		m_thread = new std::thread(&k8s_collector::get_data, &m_collector);
	}
	else
#endif // K8S_DISABLE_THREAD
	if(!in_thread)
	{
		if(!m_collector.subscription_count())
		{
			subscribe();
		}
		m_collector.get_data();
	}*/
	//***************************
	for(auto& handler : m_handlers)
	{
		if(handler.second)
		{
			handler.second->collect_data();
		}
		else
		{
			g_logger.log("K8s: " + k8s_component::get_name(handler.first) + " handler is null.", sinsp_logger::SEV_WARNING);
		}
	}
	//***************************
}
/*
void k8s_net::subscribe()
{
	for (auto& api : m_api_interfaces)
	{
		if(api.second)
		{
			m_collector.add(api.second);
		}
		else
		{
			g_logger.log("K8s: " + k8s_component::get_name(api.first) + " handler is null.", sinsp_logger::SEV_WARNING);
		}
	}
}

void k8s_net::unsubscribe()
{
	m_collector.stop();
	m_collector.remove_all();
}
*/
void k8s_net::end_thread()
{
#ifndef K8S_DISABLE_THREAD
	if(m_thread)
	{
		m_thread->join();
		delete m_thread;
		m_thread = 0;
	}
#endif
}

void k8s_net::stop_watching()
{
	if(!m_stopped)
	{
		m_stopped = true;
		//unsubscribe();
		//end_thread();
		m_collector.remove_all();
	}
}

bool k8s_net::has_dependency(const k8s_component::type_map::value_type& component)
{
	switch(component.first)
	{
		case k8s_component::K8S_NODES:
			return true;
		case k8s_component::K8S_NAMESPACES:
			{
				auto it = m_handlers.find(k8s_component::K8S_NODES);
				return it!= m_handlers.end() && it->second && it->second->is_state_built();
			}
		case k8s_component::K8S_PODS:
			{
				auto it = m_handlers.find(k8s_component::K8S_NAMESPACES);
				return it!= m_handlers.end() && it->second && it->second->is_state_built();
			}
		case k8s_component::K8S_REPLICATIONCONTROLLERS:
			{
				auto it = m_handlers.find(k8s_component::K8S_PODS);
				return it!= m_handlers.end() && it->second && it->second->is_state_built();
			}
		case k8s_component::K8S_SERVICES:
			{
				auto it = m_handlers.find(k8s_component::K8S_REPLICATIONCONTROLLERS);
				return it!= m_handlers.end() && it->second && it->second->is_state_built();
			}
		case k8s_component::K8S_REPLICASETS:
			{
				auto it = m_handlers.find(k8s_component::K8S_SERVICES);
				return it!= m_handlers.end() && it->second && it->second->is_state_built();
			}
		case k8s_component::K8S_DAEMONSETS:
			{
				auto it = m_handlers.find(k8s_component::K8S_REPLICASETS);
				return it!= m_handlers.end() && it->second && it->second->is_state_built();
			}
		case k8s_component::K8S_DEPLOYMENTS:
			{
				auto it = m_handlers.find(k8s_component::K8S_DAEMONSETS);
				return it!= m_handlers.end() && it->second && it->second->is_state_built();
			}
		case k8s_component::K8S_EVENTS:
			{
				auto it = m_handlers.find(k8s_component::K8S_SERVICES);
				return it!= m_handlers.end() && it->second && it->second->is_state_built();
			}
		case k8s_component::K8S_COMPONENT_COUNT:
		default:
			throw sinsp_exception("k8s_net::add_handler: invalid type: " +
								  component.second + " (" +
								  std::to_string(component.first) + ')');
	}
	return false;
}

void k8s_net::add_handler(const k8s_component::type_map::value_type& component)
{
	if(!has_handler(component))
	{
		if(has_dependency(component))
		{
			std::ostringstream os;
			os << m_uri.get_scheme() << "://" << m_uri.get_host();
			int port = m_uri.get_port();
			if(port) { os << ':' << port; }
			switch(component.first)
			{
				case k8s_component::K8S_NODES:
					m_handlers[component.first] = std::make_shared<k8s_node_handler>(m_state, m_collector, os.str(), "1.0", m_ssl, m_bt);
					break;
				case k8s_component::K8S_NAMESPACES:
					m_handlers[component.first] = std::make_shared<k8s_namespace_handler>(m_state, m_collector, os.str(), "1.0", m_ssl, m_bt);
					break;
				case k8s_component::K8S_PODS:
					m_handlers[component.first] = std::make_shared<k8s_pod_handler>(m_state, m_collector, os.str(), "1.0", m_ssl, m_bt);
					break;
				case k8s_component::K8S_REPLICATIONCONTROLLERS:
					m_handlers[component.first] = std::make_shared<k8s_replicationcontroller_handler>(m_state, m_collector, os.str(), "1.0", m_ssl, m_bt);
					break;
				case k8s_component::K8S_REPLICASETS:
					m_handlers[component.first] = std::make_shared<k8s_replicaset_handler>(m_state, m_collector, os.str(), "1.0", m_ssl, m_bt);
					break;
				case k8s_component::K8S_SERVICES:
					m_handlers[component.first] = std::make_shared<k8s_service_handler>(m_state, m_collector, os.str(), "1.0", m_ssl, m_bt);
					break;
				case k8s_component::K8S_DAEMONSETS:
					m_handlers[component.first] = std::make_shared<k8s_daemonset_handler>(m_state, m_collector, os.str(), "1.0", m_ssl, m_bt);
					break;
				case k8s_component::K8S_DEPLOYMENTS:
					m_handlers[component.first] = std::make_shared<k8s_deployment_handler>(m_state, m_collector, os.str(), "1.0", m_ssl, m_bt);
					break;
				case k8s_component::K8S_EVENTS:
					m_handlers[component.first] = std::make_shared<k8s_event_handler>(m_state, m_collector, os.str(), "1.0", m_ssl, m_bt, m_event_filter);
					break;
				case k8s_component::K8S_COMPONENT_COUNT:
				default:
					throw sinsp_exception("k8s_net::add_handler: invalid type: " +
										  component.second + " (" +
										  std::to_string(component.first) + ')');
			}
			g_logger.log("K8s: created " + k8s_component::get_name(component) + " handler.", sinsp_logger::SEV_INFO);
		}
		else
		{
			g_logger.log("K8s: component " + k8s_component::get_name(component) + " does not have dependencies populated yet.",
						 sinsp_logger::SEV_DEBUG);
		}
	}
	else
	{
		g_logger.log("K8s: component " + k8s_component::get_name(component) + " already exists.",
					 sinsp_logger::SEV_TRACE);
	}
}
#endif // HAS_CAPTURE
