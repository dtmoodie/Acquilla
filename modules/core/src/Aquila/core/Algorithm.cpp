#include <Aquila/core/Algorithm.hpp>
#include <Aquila/core/detail/AlgorithmImpl.hpp>

#include <MetaObject/params/IParam.hpp>
#include <MetaObject/params/InputParam.hpp>
#include <MetaObject/params/buffers/IBuffer.hpp>
#include <MetaObject/serialization/CerealMemory.hpp>
#include <MetaObject/serialization/CerealPolicy.hpp>

#include <boost/accumulators/accumulators.hpp>
#include <boost/accumulators/statistics/rolling_mean.hpp>
#include <boost/accumulators/statistics/rolling_window.hpp>

using namespace mo;
using namespace aq;

Algorithm::Algorithm() {
    _pimpl               = new impl();
    _enabled             = true;
    _pimpl->_sync_method = SyncEvery;
}

Algorithm::~Algorithm() {
    delete _pimpl;
    _pimpl = nullptr;
}

void Algorithm::setEnabled(bool value) {
    _enabled = value;
}

bool Algorithm::getEnabled() const {
    return _enabled;
}
std::vector<mo::IParam*> Algorithm::getComponentParams(const std::string& filter) const {
    std::vector<mo::IParam*> output; // = mo::IMetaObject::getParams(filter);
    for (auto& component : _algorithm_components) {
        if (component) {
            std::vector<mo::IParam*> output2 = component->getParams(filter);
            output.insert(output.end(), output2.begin(), output2.end());
        }
    }
    return output;
}
std::vector<mo::IParam*> Algorithm::getAllParams(const std::string& filter) const {
    std::vector<mo::IParam*> output = mo::IMetaObject::getParams(filter);
    for (auto& component : _algorithm_components) {
        if (component) {
            std::vector<mo::IParam*> output2 = component->getParams(filter);
            output.insert(output.end(), output2.begin(), output2.end());
        }
    }
    return output;
}
bool Algorithm::process() {
    mo::Mutex_t::scoped_lock lock(*_mtx);
    if (_enabled == false)
        return false;
    if (checkInputs() == NoneValid) {
        return false;
    }
    if (processImpl()) {
        /*_pimpl->last_ts = _pimpl->ts;
        if(_pimpl->sync_input == nullptr && _pimpl->ts != -1)
            ++_pimpl->ts;
        if(_pimpl->_sync_method == SyncEvery && _pimpl->sync_input)
        {
            if(_pimpl->_ts_processing_queue.size() && _pimpl->ts == _pimpl->_ts_processing_queue.front())
            {
                _pimpl->_ts_processing_queue.pop();
            }
        }*/
        return true;
    }
    return false;
}
mo::IParam* Algorithm::getOutput(const std::string& name) const {
    auto output = mo::IMetaObject::getOutput(name);
    if (output)
        return output;
    if (!output) {
        for (auto& component : _algorithm_components) {
            if (component) {
                output = component->getOutput(name);
                if (output)
                    return output;
            }
        }
    }
    return nullptr;
}

Algorithm::InputState Algorithm::checkInputs() {
    auto inputs = this->getInputs();
    if (inputs.size() == 0)
        return AllValid;
    for (auto input : inputs) {
        if (!input->isInputSet() && !input->checkFlags(mo::Optional_e)) {
            LOG(trace) << "Required input (" << input->getTreeName() << ") is not set to anything";
            return NoneValid;
        }
    }
    boost::optional<mo::Time_t> ts;
    boost::optional<size_t>     fn;
// First check to see if we have a sync input, if we do then use its synchronizatio method
// TODO: Handle processing queue
#ifdef _DEBUG
    struct DbgInputState {
        DbgInputState(const std::string&                 name_,
                      const boost::optional<mo::Time_t>& ts_,
                      size_t                             fn_)
            : name(name_)
            , ts(ts_)
            , fn(fn_) {}
        std::string                 name;
        boost::optional<mo::Time_t> ts;
        size_t                      fn;
    };

    std::vector<DbgInputState> input_states;
#endif
    bool buffered = false;
    if (_pimpl->sync_input) {
        ts = _pimpl->sync_input->getInputTimestamp();
        if (!ts)
            fn = _pimpl->sync_input->getInputFrameNumber();
        if (_pimpl->_sync_method == SyncEvery) {
        }
    } else {
        // first look for any direct connections, if so use the timestamp from them
        for (auto input : inputs) {
            IParam* input_param = input->getInputParam();
            if (input_param) {
                if (!input_param->checkFlags(mo::Buffer_e)) {
                    auto in_ts = input_param->getTimestamp();
#ifdef _DEBUG
                    input_states.emplace_back(input->getTreeName(), in_ts, input_param->getFrameNumber());
#endif
                    if (in_ts) {
                        if (!ts) {
                            ts = in_ts;
                            continue;
                        } else
                            ts = std::min<mo::Time_t>(*ts, *in_ts);
                    }
                } else {
                    buffered = true;
                }
            }
        }
        if (!ts && buffered && _pimpl->_buffer_timing_data.size()) {
            // Search for the smallest timestamp common to all buffers
            std::vector<mo::Time_t> tss;
            for (const auto& itr : _pimpl->_buffer_timing_data) {
                if (itr.second.size()) {
                    if (itr.second.front().ts)
                        tss.push_back(*(itr.second.front().ts));
                }
            }
            // assuming time only moves forward, pick the largest of the min timestamps
            if (tss.size()) {
                auto max_elem  = std::max_element(tss.begin(), tss.end());
                bool all_found = true;
                // Check if the value is in the other timing buffers
                for (const auto& itr : _pimpl->_buffer_timing_data) {
                    bool found = false;
                    for (const auto& itr2 : itr.second) {
                        if (itr2.ts && *(itr2.ts) == *max_elem) {
                            found = true;
                            break;
                        }
                    }
                    if (!found) {
                        all_found = false;
                        break;
                    }
                }
                if (all_found) {
                    ts = *max_elem;
                }
            }
        }

        // Check all inputs to see if any are timestamped.
        /*for(auto input : inputs){
            if(input->isInputSet()){
                if (input->checkFlags(mo::Desynced_e))
                    continue;
                auto in_ts = input->getInputTimestamp();
#ifdef _DEBUG
                input_states.emplace_back(input->getTreeName(), in_ts, 0);
#endif
                if(!in_ts)
                    continue;
                if(in_ts){
                    if(!ts){
                        ts = in_ts;
                        continue;
                    }else{
                        ts = std::min<mo::Time_t>(*ts, *in_ts);
                    }
                }
            }
        }*/
        if (!ts) {
            fn = -1;
            for (int i = 0; i < inputs.size(); ++i) {
                if (inputs[i]->isInputSet()) {
                    auto in_fn = inputs[i]->getInputFrameNumber();
                    fn         = std::min<size_t>(*fn, in_fn);
#ifdef _DEBUG
                    input_states.emplace_back(inputs[i]->getTreeName(), boost::optional<mo::Time_t>(), in_fn);
#endif
                }
            }
        }
    }
    // Synchronizing on timestamp

    if (ts) {
        size_t fn;
        for (auto input : inputs) {
            if (!input->getInput(ts, &fn)) {
                if (input->checkFlags(mo::Desynced_e))
                    if (input->getInput(boost::optional<mo::Time_t>(), &fn))
                        continue;
                if (input->checkFlags(mo::Optional_e)) {
                    // If the input isn't set and it's optional then this is ok
                    if (input->getInputParam()) {
                        // Input is optional and set, but couldn't get the right timestamp, error
                        LOG(debug) << "Failed to get input \"" << input->getTreeName() << "\" at timestamp " << ts;
                    } else {
                        LOG(trace) << "Optional input not set \"" << input->getTreeName() << "\"";
                    }
                } else {
                    // Input is not optional
                    if (auto param = input->getInputParam()) {
                        if (param->checkFlags(mo::Unstamped_e))
                            continue;
                        LOG(trace) << "Failed to get input for \"" << input->getTreeName() << "\" (" << param->getTreeName() << ") at timestamp " << ts;
                        return NoneValid;
                    } else {
                        LOG(trace) << "Input not set \"" << input->getTreeName() << "\"";
                        return NoneValid;
                    }
                }
            }
        }
        if (_pimpl->ts == ts)
            return NotUpdated;
        if (buffered && ts) {
            for (auto& itr : _pimpl->_buffer_timing_data) {
                auto itr2 = std::find_if(itr.second.begin(), itr.second.end(), [ts](const impl::SyncData& st) { if(st.ts) return *st.ts == *ts; return false; });
                if (itr2 != itr.second.end()) {
                    itr.second.erase(itr2);
                }
            }
        }
        _pimpl->ts = ts;
        _pimpl->fn = fn;
        return AllValid;
    }
    if (fn && *fn != std::numeric_limits<size_t>::max()) {
        boost::optional<mo::Time_t> ts;
        for (auto input : inputs) {
            if (!input->isInputSet() && !input->checkFlags(Optional_e)) {
                LOG(trace) << "Input not set \"" << input->getTreeName() << "\"";
                return NoneValid;
            }
            if (!input->getInput(*fn, &ts)) {
                if (input->checkFlags(Desynced_e)) {
                    continue;
                }
                if (input->checkFlags(Optional_e)) {
                    // If the input isn't set and it's optional then this is ok
                    if (input->isInputSet()) {
                        // Input is optional and set, but couldn't get the right timestamp, error
                        LOG(debug) << "Input is set to \"" << input->getTreeName() << "\" but could not get at frame number " << *fn;
                    } else {
                        LOG(trace) << "Optional input not set \"" << input->getTreeName() << "\"";
                    }
                } else {
                    // Input is not optional
                    if (auto param = input->getInputParam()) {
                        LOG(trace) << "Failed to get input for \"" << input->getTreeName() << "\" (" << param->getTreeName() << ") at framenumber "
                                   << *fn << " actual frame number " << input->getFrameNumber();
                        return NoneValid;
                    }
                }
            }
        }
        if (_pimpl->fn == fn)
            return NotUpdated;
        if (ts)
            _pimpl->ts = ts;
        _pimpl->fn     = *fn;
        return AllValid;
    }
    return NoneValid;
}

boost::optional<mo::Time_t> Algorithm::getTimestamp() {
    return _pimpl->ts;
}

void Algorithm::setSyncInput(const std::string& name) {
    _pimpl->sync_input = getInput(name);
    if (_pimpl->sync_input) {
        LOG(info) << "Updating sync parameter for " << this->GetTypeName() << " to " << name;
    } else {
        LOG(warning) << "Unable to set sync input for " << this->GetTypeName() << " to " << name;
    }
}
int Algorithm::setupVariableManager(mo::IVariableManager* mgr) {
    int count = mo::IMetaObject::setupVariableManager(mgr);
    for (auto& child : _algorithm_components) {
        count += child->setupVariableManager(mgr);
    }
    return count;
}
void Algorithm::setSyncMethod(SyncMethod _method) {
    if (_pimpl->_sync_method == SyncEvery && _method != SyncEvery) {
        //std::swap(_pimpl->_ts_processing_queue, std::queue<long long>());
        _pimpl->_ts_processing_queue = std::queue<mo::Time_t>();
    }
    _pimpl->_sync_method = _method;
}
void Algorithm::onParamUpdate(mo::IParam* param, mo::Context* ctx, mo::OptionalTime_t ts, size_t fn, mo::ICoordinateSystem* cs, mo::UpdateFlags fg) {
    mo::IMetaObject::onParamUpdate(param, ctx, ts, fn, cs, fg);
    if (_pimpl->_sync_method == SyncEvery) {
        if (param == _pimpl->sync_input) {
            auto                                ts = _pimpl->sync_input->getInputTimestamp();
            boost::recursive_mutex::scoped_lock lock(_pimpl->_mtx);
#ifdef _MSC_VER
#ifdef _DEBUG
/*if(_pimpl->_ts_processing_queue.size() && ts != (_pimpl->_ts_processing_queue.back() + 1))
                LOG(debug) << "Timestamp not monotonically incrementing.  Current: " << ts << " previous: " << _pimpl->_ts_processing_queue.back();
            auto itr = std::find(_pimpl->_ts_processing_queue._Get_container().begin(), _pimpl->_ts_processing_queue._Get_container().end(), ts);
            if(itr != _pimpl->_ts_processing_queue._Get_container().end())
            {
                LOG(debug) << "Timestamp (" << ts << ") exists in processing queue.";
            }*/
#endif
#endif
            if (_pimpl->sync_input->checkFlags(mo::Buffer_e)) {
                if (ts) {
                    _pimpl->_ts_processing_queue.push(*ts);
                } else {
                    _pimpl->_fn_processing_queue.push(_pimpl->sync_input->getInputFrameNumber());
                }
            }
        }
    } else if (_pimpl->_sync_method == SyncNewest) {
        if (param == _pimpl->sync_input) {
            _pimpl->ts = param->getTimestamp();
        }
    }
    if (fg == mo::BufferUpdated_e) {
        boost::recursive_mutex::scoped_lock lock(_pimpl->_mtx);
        mo::InputParam*                     in_param = dynamic_cast<mo::InputParam*>(param);
        auto                                itr      = _pimpl->_buffer_timing_data.find(in_param);
        if (itr == _pimpl->_buffer_timing_data.end()) {
            mo::Buffer::IBuffer* buf = dynamic_cast<mo::Buffer::IBuffer*>(in_param->getInputParam());
            _pimpl->_buffer_timing_data[in_param].set_capacity(100);
            if (buf) {
                auto capacity = buf->getFrameBufferCapacity();
                if (capacity)
                    _pimpl->_buffer_timing_data[in_param].set_capacity(*capacity);
            }
        }
        _pimpl->_buffer_timing_data[in_param].push_back(impl::SyncData(ts, fn));
    }
}

void Algorithm::setContext(const mo::ContextPtr_t& ctx, bool overwrite) {
    mo::IMetaObject::setContext(ctx, overwrite);
    for (auto& child : _algorithm_components) {
        child->setContext(ctx, overwrite);
    }
}

void Algorithm::postSerializeInit() {
    for (auto& child : _algorithm_components) {
        child->setContext(this->_ctx);
        child->postSerializeInit();
    }
}

void Algorithm::addComponent(rcc::weak_ptr<Algorithm> component) {
    _algorithm_components.push_back(component);
    mo::ISlot* slot = this->getSlot("parameter_updated", mo::TypeInfo(typeid(void(IParam*, Context*, OptionalTime_t, size_t, ICoordinateSystem*, UpdateFlags))));
    if (slot) {
        auto params = component->getParams();
        for (auto param : params) {
            param->registerUpdateNotifier(slot);
        }
    }
}

void Algorithm::Serialize(ISimpleSerializer* pSerializer) {
    mo::IMetaObject::Serialize(pSerializer);
    SERIALIZE(_algorithm_components);
}
