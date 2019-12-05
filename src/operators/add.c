#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "../pb/onnx.pb-c.h"
#include "../embeddedml_debug.h"
#include "add.h"

/*! \fn operator_add()
 *  \brief Add: Performs element-wise binary addition (with Numpy-style broadcasting support).
 *              This operator supports multidirectional (i.e., Numpy-style) broadcasting; for more
 *              details please check the doc.
 *         Version: This version of the operator has been available since version 7
 *                  of the default ONNX operator set. Other versions of this operator: Add-1, Add-6
 *         Inputs:
 *          A : T. First operand.
 *          B : T. Second operand.
 *         Outputs:
 *          C : T. Result, has same element type as two inputs
 *         Type Constraints:
 *          T : tensor(uint32), tensor(uint64), tensor(int32), tensor(int64),
 *          tensor(float16), tensor(float), tensor(double). Constrain input and output types to
 *          high-precision numeric tensors.
 *
 *  Limitations: There might be some limitations with respect to the official onnx
 *  operator. Write here possible limitations, i.e. if the function doesnt work
 *  with all types, or if it works with a specific number of dimensions only
 *
 *  \param[in]      n_input     Number of inputs of the operator
 *  \param[in]      input       Array of pointers to the inputs of the operator
 *  \param[in]      n_attribute Number of attributes of the operator
 *  \param[in]      attribute   Array of pointers to the attributes of the operator
 *  \param[in]      n_output    Numper of outputs of the operator
 *  \param[in/out]  output      Array of pointer to the outputs of the operators
 *  \return         error       Different than 0 if an error was produced
 */
int operator_add(const size_t n_input,
                 const Onnx__TensorProto **input,
                 const size_t n_attribute,
                 const Onnx__AttributeProto **attribute,
                 const size_t n_output,
                 Onnx__TensorProto **output)
{
  DEBUG_PRINT("Calling operator_add");
  debug_print_dims(input[0]->n_dims, input[0]->dims);

  if (0){
    /* TODO: Check some conditions. For example if a specific
     * functionality is not supported */
    //a->data_type == b->data_type
    //a->n_dims == b->n_dims
    //a->dims[i] == b->dims[i]
    return -1;
  }

  /* Move this block to a common function */
  output[0]->dims = malloc(input[0]->n_dims * sizeof(int64_t));
  output[0]->name         = "name_is_set_afterwards\0"; // dont do this
  output[0]->n_dims       = input[0]->n_dims;
  for (int i = 0; i < input[0]->n_dims; i++)
  {
    output[0]->dims[i] = input[0]->dims[i];
  }
  output[0]->has_raw_data = 0;
  output[0]->data_type = input[0]->data_type;

  switch(input[0]->data_type)
  {
    case ONNX__TENSOR_PROTO__DATA_TYPE__FLOAT:
    {
      output[0]->n_float_data = input[0]->n_float_data;
      output[0]->float_data = malloc(output[0]->n_float_data * sizeof(float));
      for (int i = 0; i < input[0]->n_float_data; i++) {
        /* Normal case where dimensions match */
        if (input[0]->n_dims == input[1]->n_dims) {
          output[0]->float_data[i] = input[0]->float_data[i] + input[1]->float_data[i];
        /* Broadcasting */
        }else{
          if (input[1]->n_dims == 1){
            output[0]->float_data[i] = input[0]->float_data[i] + input[1]->float_data[i%input[1]->dims[0]];
          }else{
            output[0]->float_data[i] = input[0]->float_data[i] + input[1]->float_data[i/(input[0]->dims[2]*input[0]->dims[3])];
          }
        }
      }
    } break;
    case ONNX__TENSOR_PROTO__DATA_TYPE__INT32:
      break;
    case ONNX__TENSOR_PROTO__DATA_TYPE__INT64:
      break;
    case ONNX__TENSOR_PROTO__DATA_TYPE__FLOAT16:
      break;
    case ONNX__TENSOR_PROTO__DATA_TYPE__DOUBLE:
      break;
    case ONNX__TENSOR_PROTO__DATA_TYPE__UINT32:
      break;
    case ONNX__TENSOR_PROTO__DATA_TYPE__UINT64:
      break;
    default:
      break;
  }

  debug_print_dims(output[0]->n_dims, output[0]->dims);
  return 0;
}