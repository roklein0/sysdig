//
// k8s.h
//
// extracts needed data from the k8s REST API interface
//

#pragma once

#include "json/json.h"
#include "k8s_common.h"
#include "k8s_component.h"
#include "k8s_state.h"
#include "k8s_event_data.h"
#include "k8s_net.h"
#include "sinsp_curl.h"
#include <sstream>
#include <utility>

class k8s
{
public:
#ifdef HAS_CAPTURE
	typedef sinsp_curl::ssl::ptr_t          ssl_ptr_t;
	typedef sinsp_curl::bearer_token::ptr_t bt_ptr_t;
#endif // HAS_CAPTURE

	typedef k8s_component::ext_list_ptr_t ext_list_ptr_t;
	typedef user_event_filter_t::ptr_t    filter_ptr_t;

	k8s(const std::string& uri = "http://localhost:80",
		bool start_watch = false,
		bool watch_in_thread = false,
		bool is_captured = false,
#ifdef HAS_CAPTURE
		ssl_ptr_t ssl = 0,
		bt_ptr_t bt = 0,
#endif // HAS_CAPTURE
		bool curl_debug = false,
		filter_ptr_t event_filter = nullptr,
		ext_list_ptr_t extensions = nullptr);

	~k8s();

	std::size_t count(k8s_component::type component) const;

	void check_components();

	const k8s_state_t& get_state();

	void watch();
	bool watch_in_thread() const;
	void stop_watching();

	bool is_alive() const;

#ifdef HAS_CAPTURE
	typedef k8s_state_t::event_list_t event_list_t;
	const event_list_t& get_capture_events() const { return m_state.get_capture_events(); }
	std::string dequeue_capture_event() { return m_state.dequeue_capture_event(); }
#endif // HAS_CAPTURE

	void simulate_watch_event(const std::string& json);

private:
	void stop_watch();

	void cleanup();

	bool         m_watch;
	k8s_state_t  m_state;
	filter_ptr_t m_event_filter;
	//dispatch_map m_dispatch;
	bool         m_watch_in_thread;
#ifdef HAS_CAPTURE
	k8s_net*     m_net;
#endif

	static k8s_component::type_map m_components;
	friend class k8s_test;
};

inline bool k8s::is_alive() const
{
#ifdef HAS_CAPTURE
	ASSERT(m_net);
	return m_net->is_healthy();
#endif
	return true;
}

inline bool k8s::watch_in_thread() const
{
	return m_watch_in_thread;
}
