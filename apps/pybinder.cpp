//
// Created by Unravel on 2022/4/20.
//
#include "CPGGen.h"
#include "WFDGGen.h"
#include "WFDGGen/WFDG.h"
#include "pybind11/pybind11.h"
#include "pybind11/stl.h"

namespace wfdg {
    namespace py = pybind11;

    PYBIND11_MODULE(wfdg_generator, m) {
        py::class_<Configuration>(m, "Configuration")
                .def(py::init<>())
                .def_readwrite("dest_func", &Configuration::destFunc)
                .def_readwrite("no_sensitive", &Configuration::noSensitive)
                .def_readwrite("sensitive_line", &Configuration::sensitiveLine)
                .def_readwrite("debug", &Configuration::debug)
                .def_readwrite("key_words", &Configuration::keyWords)
                .def_readwrite("AST_Stmt_kinds", &Configuration::ASTStmtKindMap)
                .def_readwrite("use_weight", &Configuration::useWeight)
                .def_readwrite("weight_pred_ratio", &Configuration::weightPredRatio)
                .def_readwrite("weight_succ_ratio", &Configuration::weightSuccRatio)
                .def_readwrite("graph_pred_depth", &Configuration::graphPredDepth)
                .def_readwrite("graph_succ_depth", &Configuration::graphSuccDepth)
                .def("specify_func", &Configuration::specifyFunc)
                .def("add_keywords", &Configuration::addKeyWords);

        py::class_<WFDG> w(m, "WFDG");
        w.def(py::init<std::string, unsigned>())
            .def_property_readonly("__func_name", &WFDG::getFuncName)
            .def("get_func_name", &WFDG::getFuncName)
            .def_property_readonly("__root_line", &WFDG::getRootLine)
            .def("get_root_line", &WFDG::getRootLine)
            .def_property_readonly("__nodes", &WFDG::getNodes)
            .def("add_node", &WFDG::addNode)
            .def("get_node", &WFDG::getNode)
            .def("get_nodes", &WFDG::getNodes)
            .def("get_node_cnt", &WFDG::getNodeCnt)
            .def_property_readonly("__edges", &WFDG::getEdges)
            .def_property_readonly("__depn_edges", &WFDG::getDepnEdges)
            .def_property_readonly("__all_edges", &WFDG::getAllEdges)
            .def("set_all_edges", &WFDG::setAllEdges)
            .def("get_all_edges", &WFDG::getAllEdges)
            .def("get_all_edge_cnt", &WFDG::getAllEdgeCnt)
            .def("to_string", &WFDG::toString)
            .def("to_json", &WFDG::toJson);

        py::class_<WFDG::WFDGNode>(w, "WFDGNode")
                .def(py::init<>())
                .def_readwrite("id", &WFDG::WFDGNode::id)
                .def_readwrite("depn_weight", &WFDG::WFDGNode::depnWeight)
                .def_readwrite("node_weight", &WFDG::WFDGNode::nodeWeight)
                .def_readwrite("weight", &WFDG::WFDGNode::weight)
                .def_readwrite("stmt_vec", &WFDG::WFDGNode::stmtVec)
                .def("to_string", &WFDG::WFDGNode::toString)
                .def("to_json", &WFDG::WFDGNode::toJson);

        m.def("gen_WFDGs", &genWFDGs);
    }

}

