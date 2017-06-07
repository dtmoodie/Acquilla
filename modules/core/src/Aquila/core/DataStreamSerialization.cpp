#include "Aquila/core/DataStream.hpp"
#include "Aquila/nodes/Node.hpp"
#include "Aquila/serialization/cereal/JsonArchive.hpp"
//#include <Aquila/serialization/cereal/memory.hpp>
#include "MetaObject/serialization/Policy.hpp"
#include "MetaObject/serialization/SerializationFactory.hpp"
#include "MetaObject/serialization/Serializer.hpp"

#include <cereal/archives/json.hpp>
#include <cereal/types/map.hpp>
#include <cereal/types/tuple.hpp>
#include <cereal/types/utility.hpp>
#include <cereal/types/vector.hpp>

#include <boost/filesystem.hpp>
#include <fstream>

using namespace aq;
template <typename AR>
void DataStream::load(AR& ar)
{
    this->_load_params<AR>(ar, mo::_counter_<_DS_N_ - 1>());
    for (auto& node : top_level_nodes) {
        node->setDataStream(this);
    }
    this->_load_parent<AR>(ar);
}

template <typename AR>
void DataStream::save(AR& ar) const
{
    ObjectId id = GetObjectId();
    std::string type = GetTypeName();
    ar(cereal::make_nvp("TypeId", id.m_ConstructorId));
    ar(cereal::make_nvp("InstanceId", id.m_PerTypeId));
    ar(cereal::make_nvp("TypeName", type));
    this->_save_params<AR>(ar, mo::_counter_<_DS_N_ - 1>());
    this->_save_parent<AR>(ar);
}

std::vector<IDataStream::Ptr> IDataStream::load(const std::string& config_file)
{
    VariableMap vm, sm;
    return load(config_file, vm, sm);
}

std::vector<IDataStream::Ptr> IDataStream::load(const std::string& config_file, VariableMap& vm, VariableMap& sm)
{
    /*rcc::shared_ptr<DataStream> stream_ = rcc::shared_ptr<DataStream>::create();
    if(!stream_)
    {
        LOG(error) << "Unable to create data stream";
        return Ptr();
    }
    stream_->stopThread();
    stream_->top_level_nodes.clear();*/
    std::vector<rcc::shared_ptr<IDataStream> > streams; //(stream_);
    if (!boost::filesystem::exists(config_file)) {
        LOG(warning) << "Stream config file doesn't exist: " << config_file;
        return {};
    }
    std::string ext = boost::filesystem::extension(config_file);
    if (ext == ".bin") {
        std::ifstream ifs(config_file, std::ios::binary);
        cereal::BinaryInputArchive ar(ifs);
        //ar(stream);
        return streams;
    } else if (ext == ".json") {
        try {
            std::ifstream ifs(config_file, std::ios::binary);
            aq::JSONInputArchive ar(ifs, vm, sm);
            VariableMap defaultVM, defaultSM;
            ar(cereal::make_optional_nvp("DefaultVariables", defaultVM, defaultVM));
            ar(cereal::make_optional_nvp("DefaultStrings", defaultSM, defaultSM));
            for (const auto& pair : defaultVM) {
                if (vm.count(pair.first) == 0)
                    vm[pair.first] = pair.second;
            }
            for (const auto& pair : defaultSM) {
                if (sm.count("${" + pair.first + "}") == 0)
                    sm["${" + pair.first + "}"] = pair.second;
            }
            if (sm.size()) {
                std::stringstream ss;
                for (const auto& pair : sm)
                    ss << "\n"
                       << pair.first << " = " << pair.second;
                LOG(debug) << "Used string replacements: " << ss.str();
            }

            if (vm.size()) {
                std::stringstream ss;
                for (const auto& pair : vm)
                    ss << "\n"
                       << pair.first << " = " << pair.second;
                LOG(debug) << "Used variable replacements: " << ss.str();
            }

            auto dsnvp = cereal::make_optional_nvp("DataStreams", streams);
            ar(dsnvp);
            if (!dsnvp.success) {
                LOG(warning) << "Attempting to load old config file format";
                rcc::shared_ptr<IDataStream> stream;
                ar(cereal::make_optional_nvp("value0", stream));
                if (stream) {
                    streams.push_back(stream);
                }
            }
        } catch (cereal::RapidJSONException& e) {
            LOG(warning) << "Unable to parse " << config_file << " due to " << e.what();
            return {};
        }
        return streams;
    } else if (ext == ".xml") {
        std::ifstream ifs(config_file, std::ios::binary);
        cereal::XMLInputArchive ar(ifs);
        //ar(stream);
        return streams;
    }
    return streams;
}

void IDataStream::save(const std::string& config_file, rcc::shared_ptr<IDataStream>& stream)
{
    std::vector<rcc::shared_ptr<IDataStream> > streams;
    streams.push_back(stream);
    save(config_file, streams);
}
void IDataStream::save(const std::string& config_file, std::vector<rcc::shared_ptr<IDataStream> >& streams)
{
    save(config_file, streams, VariableMap(), VariableMap());
}

void IDataStream::save(const std::string& config_file, std::vector<rcc::shared_ptr<IDataStream> >& streams,
    const VariableMap& vm, const VariableMap& sm)
{
    for (auto& stream : streams)
        stream->stopThread();
    if (boost::filesystem::exists(config_file)) {
        LOG(info) << "Stream config file exists, overwiting: " << config_file;
    }
    std::string ext = boost::filesystem::extension(config_file);
    if (ext == ".json") {
        try {
            std::ofstream ofs(config_file, std::ios::binary);
            std::map<std::string, std::string> write_sm;
            if (sm.size()) {
                for (const auto& pair : sm) {
                    write_sm[pair.first.substr(2, pair.first.size() - 3)] = pair.second;
                }
                //ar(cereal::make_nvp("DefaultStrings", write_sm));
            }
            aq::JSONOutputArchive ar(ofs, aq::JSONOutputArchive::Options(), vm, write_sm);
            if (vm.size()) {
                //ar(cereal::make_nvp("DefaultVariables", vm));
            }

            ar(cereal::make_nvp("DataStreams", streams));
        } catch (cereal::RapidJSONException& e) {
            LOG(warning) << "Unable to save " << config_file << " due to " << e.what();
        }
    }
}

void HandleNode(cereal::JSONInputArchive& ar, rcc::shared_ptr<Nodes::Node>& node,
    std::vector<std::pair<std::string, std::string> >& inputs)
{
    ar.startNode();
    std::string name;
    std::string type;
    ar(CEREAL_NVP(name));
    ar(CEREAL_NVP(type));
    ar.startNode();
    node = mo::MetaObjectFactory::instance()->create(type.c_str());
    if (!node) {
        LOG(warning) << "Unable to create node with type: " << type;
        return;
    }
    node->setTreeName(name);
    auto parameters = node->getParams();
    for (auto param : parameters) {
        if (param->checkFlags(mo::Output_e) || param->checkFlags(mo::Input_e))
            continue;
        auto func1 = mo::SerializationFactory::instance()->getJsonDeSerializationFunction(param->getTypeInfo());
        if (func1) {
            if (!func1(param, ar)) {
                LOG(info) << "Unable to deserialize " << param->getName();
            }
        }
    }

    ar.finishNode();
    try {
        ar(CEREAL_NVP(inputs));
    } catch (...) {
    }

    ar.finishNode();
}

bool DataStream::loadStream(const std::string& filename)
{
    return false;
}

struct NodeSerializationInfo {
    std::string name;
    std::string type;
    std::vector<mo::IParam*> parameters;
    std::vector<std::pair<std::string, std::string> > inputs;
    void save(cereal::JSONOutputArchive& ar) const
    {
        ar(CEREAL_NVP(name));
        ar(CEREAL_NVP(type));
        ar.setNextName("parameters");
        ar.startNode();
        for (int i = 0; i < parameters.size(); ++i) {
            auto func1 = mo::SerializationFactory::instance()->getJsonSerializationFunction(parameters[i]->getTypeInfo());
            if (func1) {
                if (!func1(parameters[i], ar)) {
                    LOG(debug) << "Unable to serialize " << parameters[i]->getTreeName();
                }
            }
        }
        ar.finishNode();
        ar(CEREAL_NVP(inputs));
    }
    template <class AR>
    void save(AR& ar) const {}
    template <class AR>
    void load(AR& ar) {}
};

void PopulateSerializationInfo(Nodes::Node* node, std::vector<NodeSerializationInfo>& info)
{
    bool found = false;
    for (int i = 0; i < info.size(); ++i) {
        if (node->getTreeName() == info[i].name)
            found = true;
    }
    if (!found) {
        NodeSerializationInfo node_info;
        node_info.name = node->getTreeName();
        node_info.type = node->GetTypeName();
        auto all_params = node->getParams();
        for (auto& param : all_params) {
            if (param->getName() == "_dataStream" || param->getName() == "_children" || param->getName() == "_parents" || param->getName() == "_unique_id")
                continue;
            if (param->checkFlags(mo::Control_e)) {
                node_info.parameters.push_back(param);
            }
            if (param->checkFlags(mo::Input_e)) {
                std::string input_name;
                mo::InputParam* input_param = dynamic_cast<mo::InputParam*>(param);
                if (input_param) {
                    mo::IParam* _input_param = input_param->getInputParam();
                    if (_input_param) {
                        input_name = _input_param->getTreeName();
                    }
                }
                node_info.inputs.emplace_back(param->getName(), input_name);
            }
        }
        info.push_back(node_info);
    }
    auto children = node->getChildren();
    for (auto child : children) {
        PopulateSerializationInfo(child.get(), info);
    }
}

bool DataStream::saveStream(const std::string& filename)
{
    if (boost::filesystem::exists(filename)) {
        LOG(warning) << "Overwriting existing stream config file: " << filename;
    }

    std::string ext = boost::filesystem::extension(filename);
    if (ext == ".bin") {
        mo::StartSerialization();
        std::ofstream ofs(filename, std::ios::binary);
        cereal::BinaryOutputArchive ar(ofs);
        ar(*this);
        mo::EndSerialization();
        return true;
    } else if (ext == ".json") {
        mo::StartSerialization();
        std::ofstream ofs(filename, std::ios::binary);
        cereal::JSONOutputArchive ar(ofs);
        std::vector<NodeSerializationInfo> serializationInfo;
        for (auto& node : top_level_nodes) {
            PopulateSerializationInfo(node.get(), serializationInfo);
        }
        ar(cereal::make_nvp("nodes", serializationInfo));
        //ar(*this);
        mo::EndSerialization();
        return true;
    } else if (ext == ".xml") {
        mo::StartSerialization();
        std::ofstream ofs(filename, std::ios::binary);
        cereal::XMLOutputArchive ar(ofs);
        ar(*this);
        mo::EndSerialization();
        return true;
    }
    return false;
}
