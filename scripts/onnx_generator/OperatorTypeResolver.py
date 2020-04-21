import inspect
import os
import pathlib

class OperatorTypeSwitch:
    _template_resolveType = '''
uint32_t {constraint} = 0;
{{ // get type of constraint '{constraint}'
    const char name = "{name}";
    Onnx__TensorProto *tensor = NULL;
    size_t n_tensor = operator_findTensors(&tensor, &name, 1, {inOrOutput}, n_{inOrOutput});
    if (n_tensor == 0) {{
        fprintf(stderr,"tensor '%s' not found!", name);
        exit(1);
    }}
    {constraint} = tensor->data_type;
}}
'''
    _template_switch = '''
switch ( {constraint} ) {{
    {cases}
    default: {{
        fprintf(stderr, "no matching type for constraint '{constraint}' found!\\n");
        exit(1);
    }}
}}
'''
    _template_case = '''
case {case}: {content}
'''

    _template = '''
{{
    {resolveTypes}
    {switch}
}}
'''
    def __init__(self, schema):
        self.schema = schema

    def __str__(self):
        return self.text()

    def __repr__(self):
        return f"OperatorTypeSwitch({self.schema.__repr__()})"

    def text(self, indent=4):
        resolveTypes = []
        cases = []

        for constraint in self.schema.constraints.keys():
            for input in self.schema.inputs:
                if constraint == input.constraint:
                    resolveTypes.append(self._template_resolveType.format(
                        constraint = constraint,
                        inOrOutput = "input",
                        name = input.name,
                    ).strip())
                    break
            else:
                for output in self.schema.outputs:
                    if constraint == output.constraint:
                        resolveTypes.append(self._template_resolveType.format(
                            constraint = constraint,
                            inOrOutput = "output",
                            name = output.name,
                        ).strip())
                        break

        permutationsMap = self.schema.constraints.typePermutationsMap()
        if not permutationsMap:
            return "/* skipping constraint test, because no constraint exist */"
        return self._template.format(
            resolveTypes = "\n".join(resolveTypes).strip().replace("\n",f"\n{' '*indent}"),
            switch = self._text_walkPermutationsMap(permutationsMap, indent).replace('\n','\n'+' '*indent)
        ).strip()

    def _text_walkPermutationsMap(self, node, indent=4):
        cases = []
        for k,v in node.items():
            case = k[-1][1].onnxTensorDataTypes()
            if not case:
                cases.append(f"/* skip non tensor constraint '{k[-1][0]}' ('{k[-1][1].original}') */")
                continue
            operator_name = self.schema.operator_name
            typePermutationText = self.schema.constraints.typePermutationText(k)
            if not v:
                cases.append(self._template_case.format(
                    case = case[0],
                    content = f"return &{operator_name}__{typePermutationText};"
                ).strip())
            else:
                cases.append(self._template_case.format(
                    case = case[0],
                    content = self._text_walkPermutationsMap(v,indent)
                ).strip())
        return self._template_switch.format(
            constraint = list(node.keys())[0][-1][0],
            cases = "\n".join(cases).replace('\n','\n'+' '*indent)
        ).strip()

class OperatorTypeResolver:
    _template = '''
//this file was generated by {script}
#include "{operator_name}.h"
#include <inttypes.h>
#include <stdio.h>

onnx_operator {operator_name}_resolveType(
    size_t                  n_input,
    Onnx__TensorProto    ** input,
    size_t                  n_attribute,
    Onnx__AttributeProto ** attribute,
    size_t                  n_output,
    Onnx__TensorProto    ** output
){{
    {switch}
}}
'''

    def __init__(self, schema, path):
        self.schema = schema
        self.path = path
        self.switch = OperatorTypeSwitch(self.schema).text()

    def text(self):
      return self._template.format(
        script=self._rel_path(inspect.getfile(inspect.currentframe())),
        operator_name=self.schema.operator_name,
        switch=self.switch
      )

    def filename(self, path):
        path = str(path)
        path += f"/{self.schema.domain}"
        path += f"/{self.schema.operator_name}_resolveType.c"
        return pathlib.Path(path)

    def _rel_path(self, path):
        return os.path.relpath(os.path.realpath(path),os.path.realpath(self.path))

    def __str__(self):
        return self.text()

    def __repr__(self):
        return f"OperatorTypeResolver({self.schema.__repr__()}, {self.path.__repr__()})"
