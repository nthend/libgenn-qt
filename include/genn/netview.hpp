#pragma once

#include <random>
#include <mutex>

#include <QGraphicsView>
#include <QGraphicsScene>
#include <QGraphicsItem>
#include <QWheelEvent>
#include <QTimer>

#include <genn/genetics.hpp>

#include <la/vec.hpp>

#include "nodeview.hpp"
#include "linkview.hpp"

class NetView : public QGraphicsView {
	Q_OBJECT
public:
	QGraphicsScene *scene;
	std::map<NodeID, NodeView *> nodes;
	std::map<LinkID, LinkView *> links;
	
	std::minstd_rand re;
	std::uniform_real_distribution<> unif;
	
	bool done = true;
	const Network *net;
	
	std::mutex mtx;
	NetView(const Network *n);

	virtual ~NetView();
	
private:
	void init(NodeID id, NodeView *nv);
	void init(LinkID id, LinkView *lv);
	
	template <typename K, typename VS, typename VD>
	void sync_map(const std::map<K, VS> &src, std::map<K, VD*> &dst) {
		for(auto &p : dst) {
			p.second->exist = false;
		}
		
		for(const auto &ep : src) {
			const K &id = ep.first;
			const VS &e = ep.second;
			VD *it = nullptr;
			auto ii = dst.find(id);
			if(ii == dst.end()) {
				it = new VD();
				init(id, it);
				ii = dst.insert(std::make_pair(id, it)).first;
				scene->addItem(it);
			} else {
				it = ii->second;
			}
			it->sync(e);
			it->exist = true;
		}
		
		for(auto ii = dst.begin(); ii != dst.end();) {
			if(ii->second->exist) {
				++ii;
			} else {
				auto iv = ii->second;
				scene->removeItem(iv);
				dst.erase(ii++);
				delete iv;
			}
		}
	}
	
	void timer_func() {
		int ms = 40;
		
		sync(net);
		move(1e-3*ms);
		update();
		
		if(!done) {
			QTimer::singleShot(ms, [this](){timer_func();});
		}
	}
	
public:
	void startAnim() {
		done = false;
		timer_func();
	}
	
	void stopAnim() {
		done = true;
	}
	
	void sync(const Network *net) {
		mtx.lock();
		sync_map(net->nodes, nodes);
		sync_map(net->links, links);
		mtx.unlock();
	}
	
	void move(double dt);
	void initView();
	void fit();
	virtual void resizeEvent(QResizeEvent *event) override;
};
