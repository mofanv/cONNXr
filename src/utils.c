#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include "onnx.pb-c.h"
#include "utils.h"
#include "tracing.h"

// TODO Rethink this function?
Onnx__TensorProto* searchTensorProtoByName(Onnx__ModelProto *model,
                                           Onnx__TensorProto **inputs,
                                           int nInputs,
                                           char *name)
{
  TRACE_ENTRY(1);
  TRACE(1, true, "Searching for TensorProto with name=%s", name);

  // Search in initializers
  for (int initializer = 0; initializer < model->graph->n_initializer; initializer++)
  {
    if (!strcmp(model->graph->initializer[initializer]->name, name))
    {
      TRACE(1, true, "Found TensorProto in initializer list with name=%s", model->graph->initializer[initializer]->name);
      TRACE_EXIT(1);
      return model->graph->initializer[initializer];
    }
  }

  // Search in inputs to the model
  for (int inIdx = 0; inIdx < nInputs; inIdx++)
  {
    if (!strcmp(inputs[inIdx]->name, name))
    {
      TRACE(1, true, "Found TensorProto in inputs to de model with name=%s", inputs[inIdx]->name);
      TRACE_EXIT(1);
      return inputs[inIdx];
    }
  }

  // Search in new context. Only for outputs
  if (_populatedIdx != -1)
  {
    // Iterate all populated nodes
    for (int node_i = 0; node_i < _populatedIdx+1; node_i++)
    {
      for (int output_i = 0; output_i < all_context[node_i].onnx_node->n_output; output_i++){
        TRACE(1, true, "Searching %s, found %s", name, all_context[node_i].outputs[output_i]->name);
        if (!strcmp(all_context[node_i].outputs[output_i]->name, name))
        {
          TRACE(1, true, "Found TensorProto in outputs from new context name=%s", all_context[node_i].outputs[output_i]->name);
          TRACE_EXIT(1);
          return all_context[node_i].outputs[output_i];
        }
      }
    }
  }

  TRACE_WARN(1, true, "%s was not found anywhere, maybe you should worry", name);
  TRACE_EXIT(1);
  return NULL;
}

Onnx__TensorProto* searchInputByName(node_context *ctx,
                                     int index)
{
  TRACE_ENTRY(1);
  // Just return null if we are accesing an optional parameters that is not present
  if (index > ctx->onnx_node->n_input-1)
  {
    TRACE_WARN(1, true, "index (%d) exceeds number of inputs (%zu)!", index, ctx->onnx_node->n_input);
    TRACE_EXIT(1);
    return NULL;
  }

  // Just return null if input name is empty (marked as skipped)
  if (!*ctx->onnx_node->input[index]) {
    TRACE_WARN(1, true, "index (%d) specifies skipped input!", index);
    TRACE_EXIT(1);
    return NULL;
  }

  for (int i = 0; i < ctx->onnx_node->n_input; i++)
  {
    if (!ctx->inputs[i]) {
      continue;
    }
    TRACE(2, true, "Searching inputs %s, %s", ctx->inputs[i]->name, ctx->onnx_node->input[index]);
    if (!strcmp(ctx->inputs[i]->name, ctx->onnx_node->input[index]))
    {
      TRACE_EXIT(1);
      return ctx->inputs[i];
    }
  }
  TRACE_WARN(1, true, "%s not found", ctx->onnx_node->input[index]);
  TRACE_EXIT(1);
  return NULL;
}

Onnx__TensorProto* searchOutputByName(node_context *ctx,
                                      int index)
{
  TRACE_ENTRY(1);
  //TODO skipped outputs, see inputs!
  // Just return null if we are accesing an optional parameters that is not present
  if (index > ctx->onnx_node->n_output-1)
  {
    TRACE_WARN(1, true, "index (%d) exceeds number of outputs (%zu)!", index, ctx->onnx_node->n_output);
    TRACE_EXIT(1);
    return NULL;
  }

  for (int i = 0; i < ctx->onnx_node->n_output; i++)
  {
    if (!strcmp(ctx->outputs[i]->name, ctx->onnx_node->output[index]))
    {
      TRACE_EXIT(1);
      return ctx->outputs[i];
    }
  }
  TRACE_WARN(1, true, "%s not found", ctx->onnx_node->output[index]);
  TRACE_EXIT(1);
  return NULL;
}

Onnx__AttributeProto* searchAttributeNyName( size_t n_attribute,
                                             Onnx__AttributeProto **attribute,
                                             char *name)
{
  TRACE_ENTRY(1);
  TRACE(1, true, "Searching for AttributeProto with name=%s", name);
  for (int i = 0; i < n_attribute; i++){
    if (!strcmp(attribute[i]->name, name)){
      TRACE(1, true, "Attribute %s was found", name);
      TRACE_EXIT(1);
      return attribute[i];
    }
  }
  TRACE_WARN(1, true, "Attribute %s was NOT found", name);
  TRACE_EXIT(1);
  return NULL;
}


Onnx__ModelProto* openOnnxFile(char *fname){
  TRACE_ENTRY(1);
  Onnx__ModelProto *model = NULL;

  FILE *fl = fopen(fname, "r");
  if (fl == NULL){
    TRACE_ERROR(0, true, "File was not opened");
    TRACE_EXIT(1);
    return model;
  }

  fseek(fl, 0, SEEK_END);
  long len = ftell(fl);
  uint8_t *ret = malloc(len);
  fseek(fl, 0, SEEK_SET);
  for(long read = 0; read < len; read += fread(ret, 1, len-read, fl));
  fclose(fl);

  TRACE(1, true, "length of file is %ld", len);

  model = onnx__model_proto__unpack(NULL,len,ret);

  TRACE_EXIT(1);
  return model;
}

size_t saveOnnxFile(Onnx__ModelProto *model, char *fname){
  
  long len;
  uint8_t *ret;

  len = onnx__model_proto__get_packed_size(model);
  ret = malloc(len);

  onnx__model_proto__pack(model, ret);
  //fprintf(stderr,"Writing %ld serialized bytes\n",len);

  FILE *fl = fopen(fname, "w");
  
  for(long write = 0; write < len; write += fwrite(ret, 1, len-write, fl));
  fclose(fl);

  return 1;
}


Onnx__TensorProto *openTensorProtoFile(char *fname){
  TRACE_ENTRY(1);
  Onnx__TensorProto *model = NULL;

  TRACE(1, true, "Opening .pb file %s", fname);

  FILE *fl = fopen(fname, "r");
  if (fl == NULL){
    TRACE_ERROR(0, true, "File was not opened");
    TRACE_EXIT(1);
    return model;
  }

  fseek(fl, 0, SEEK_END);
  long len = ftell(fl);
  uint8_t *ret = malloc(len);
  fseek(fl, 0, SEEK_SET);
  for(long read = 0; read < len; read += fread(ret, 1, len-read, fl));
  fclose(fl);

  TRACE(1, true, "length of file %ld", len);

  model = onnx__tensor_proto__unpack(NULL,len,ret);

  TRACE_EXIT(1);
  return model;
}

/*
Takes as an input a tensor with has_raw_data field TRUE, reads raw_data and
stores it into "formated" data in the corresponding field. Hardcoded for float
*/
int convertRawDataOfTensorProto(Onnx__TensorProto *tensor)
{
  TRACE_ENTRY(1);
  if (tensor == NULL)
  {
    TRACE_WARN(1, true, "Tensor is null, break");
    TRACE_EXIT(1);
    return 0;
  }

  if (!tensor->has_raw_data)
  {
    TRACE_WARN(1, true, "Input tensor doesnt have raw_data, doing nothing");
    TRACE_EXIT(1);
    return 0;
  }

  TRACE(1, true, "Tensor has raw data. Unserializing it");
  TRACE(1, true, "Tensor type = %d", tensor->data_type);

  switch(tensor->data_type)
  {
    case ONNX__TENSOR_PROTO__DATA_TYPE__UNDEFINED:
      break;
    case ONNX__TENSOR_PROTO__DATA_TYPE__FLOAT:
    {
      tensor->n_float_data = tensor->raw_data.len/4;
      // todo use sizeof
      tensor->float_data = malloc(tensor->n_float_data * sizeof(float));
      for (int i = 0; i < tensor->raw_data.len; i+=4)
      {
        // Once float is 4 bytes.
        tensor->float_data[i/4] = *(float *)&tensor->raw_data.data[i];
        //TRACE_LEVEL0("tensor->float_data[%d] = %f", i/4, tensor->float_data[i/4]);
      }
    } break;
    /* TODO I think uint8 and 16/32 can be all merged since the type is the same*/
    case ONNX__TENSOR_PROTO__DATA_TYPE__UINT8:
      tensor->n_int32_data = tensor->raw_data.len/sizeof(uint8_t);
      tensor->int32_data = malloc(tensor->n_int32_data * sizeof(int32_t));
      for (int i = 0; i < tensor->raw_data.len; i+=sizeof(uint8_t)){
        tensor->int32_data[i/sizeof(uint8_t)] = *(uint8_t *)&tensor->raw_data.data[i];
      }
      break;
    case ONNX__TENSOR_PROTO__DATA_TYPE__INT8:
      break;
    case ONNX__TENSOR_PROTO__DATA_TYPE__UINT16:
      break;
    case ONNX__TENSOR_PROTO__DATA_TYPE__INT16:
      break;
    case ONNX__TENSOR_PROTO__DATA_TYPE__INT32:
      break;
    case ONNX__TENSOR_PROTO__DATA_TYPE__INT64:
    {
      tensor->n_int64_data = tensor->raw_data.len/sizeof(int64_t);
      tensor->int64_data = malloc(tensor->n_int64_data * sizeof(int64_t));
      for (int i = 0; i < tensor->raw_data.len; i+=sizeof(int64_t))
      {
        tensor->int64_data[i/sizeof(int64_t)] = *(int64_t *)&tensor->raw_data.data[i];
        //TRACE_LEVEL0("tensor->int64_data[%lu] = %lld", i/sizeof(int64_t), tensor->int64_data[i/sizeof(int64_t)]);
      }
    } break;
    case ONNX__TENSOR_PROTO__DATA_TYPE__STRING:
      break;
    case ONNX__TENSOR_PROTO__DATA_TYPE__BOOL:
      break;
    case ONNX__TENSOR_PROTO__DATA_TYPE__FLOAT16:
      break;
    case ONNX__TENSOR_PROTO__DATA_TYPE__DOUBLE:
      break;
    case ONNX__TENSOR_PROTO__DATA_TYPE__UINT32:
      break;
    case ONNX__TENSOR_PROTO__DATA_TYPE__UINT64:
      break;
    case ONNX__TENSOR_PROTO__DATA_TYPE__COMPLEX64:
      break;
    case ONNX__TENSOR_PROTO__DATA_TYPE__COMPLEX128:
      break;
    case ONNX__TENSOR_PROTO__DATA_TYPE__BFLOAT16:
      break;
    default:
      break;
  }

  // TODO is this allowed?
  //free(tensor->raw_data.data);
  /* Set this to avoid unserializing again */
  tensor->has_raw_data = 0;
  //tensor->raw_data.len = 0;

  TRACE_EXIT(1);

  return 0;
}

/* tp has already memory allocated for the struct. Maybe its
memory should be allocated here instead. This is a bit experimental TODO:*/
/* this is crap, remove. not used*/
void mallocTensorProto(Onnx__TensorProto *tp,
                       Onnx__TensorProto__DataType data_type,
                       size_t n_dims,
                       size_t n_data)
{
  TRACE_ENTRY(1);

  tp->has_raw_data = 0;
  tp->n_external_data = 0;
  tp->has_data_location = 0;
  tp->n_dims = n_dims;
  tp->dims = malloc(n_dims * sizeof(int64_t));
  tp->name = malloc(MAX_CHAR_SIZE * sizeof(char));

  if (tp->data_type == ONNX__TENSOR_PROTO__DATA_TYPE__FLOAT ||
      tp->data_type == ONNX__TENSOR_PROTO__DATA_TYPE__COMPLEX64){
    tp->n_float_data = n_data;
    tp->float_data = malloc(n_data * sizeof(float));

  }else if (tp->data_type == ONNX__TENSOR_PROTO__DATA_TYPE__INT32  ||
            tp->data_type == ONNX__TENSOR_PROTO__DATA_TYPE__INT16  ||
            tp->data_type == ONNX__TENSOR_PROTO__DATA_TYPE__INT8   ||
            tp->data_type == ONNX__TENSOR_PROTO__DATA_TYPE__UINT16 ||
            tp->data_type == ONNX__TENSOR_PROTO__DATA_TYPE__UINT8  ||
            tp->data_type == ONNX__TENSOR_PROTO__DATA_TYPE__BOOL   ||
            tp->data_type == ONNX__TENSOR_PROTO__DATA_TYPE__FLOAT16){
    tp->n_int32_data = n_data;
    tp->int32_data = malloc(n_data * sizeof(int32_t));

  }else if (tp->data_type == ONNX__TENSOR_PROTO__DATA_TYPE__INT64){
    tp->n_int64_data = n_data;
    tp->int64_data = malloc(n_data * sizeof(int64_t));

  }else if(tp->data_type == ONNX__TENSOR_PROTO__DATA_TYPE__STRING){
    /* TODO
    size_t n_string_data;
    ProtobufCBinaryData *string_data; */

  }else if(tp->data_type == ONNX__TENSOR_PROTO__DATA_TYPE__DOUBLE ||
          tp->data_type == ONNX__TENSOR_PROTO__DATA_TYPE__COMPLEX128){
    tp->n_double_data = n_data;
    tp->double_data = malloc(n_data * sizeof(double));

  }else if(tp->data_type == ONNX__TENSOR_PROTO__DATA_TYPE__UINT64 ||
          tp->data_type == ONNX__TENSOR_PROTO__DATA_TYPE__UINT32){
    tp->n_uint64_data = n_data;
    tp->uint64_data = malloc(n_data * sizeof(uint64_t));
  }

  TRACE_EXIT(1);
}

void init_tensor_proto(Onnx__TensorProto *tp){

  TRACE_ENTRY(1);
  /* All fields are the following
  ProtobufCMessage base;
  size_t n_dims;
  int64_t *dims;
  protobuf_c_boolean has_data_type;
  int32_t data_type;
  Onnx__TensorProto__Segment *segment;
  size_t n_float_data;
  float *float_data;
  size_t n_int32_data;
  int32_t *int32_data;
  size_t n_string_data;
  ProtobufCBinaryData *string_data;
  size_t n_int64_data;
  int64_t *int64_data;
  char *name;
  char *doc_string;
  protobuf_c_boolean has_raw_data;
  ProtobufCBinaryData raw_data;
  size_t n_external_data;
  Onnx__StringStringEntryProto **external_data;
  protobuf_c_boolean has_data_location;
  Onnx__TensorProto__DataLocation data_location;
  size_t n_double_data;
  double *double_data;
  size_t n_uint64_data;
  uint64_t *uint64_data;
  */
  //tp->base = xx;
  onnx__tensor_proto__init(tp);
  tp->n_dims = 0;
  tp->dims = NULL;
  tp->has_data_type = 1;
  tp->data_type = 0;
  tp->segment = NULL;
  tp->n_float_data = 0;
  tp->float_data = NULL;
  tp->n_int32_data = 0;
  tp->int32_data = NULL;
  tp->n_string_data = 0;
  tp->string_data = NULL;
  tp->n_int64_data = 0;
  tp->int64_data = NULL;
  tp->name = NULL;
  tp->doc_string = NULL;
  tp->has_raw_data = 0;
  //tp->raw_data = xx;
  tp->n_external_data = 0;
  tp->external_data = NULL;
  tp->has_data_location = 0;
  //tp->data_location = xx;
  tp->n_double_data = 0;
  tp->double_data = NULL;
  tp->n_uint64_data = 0;
  tp->uint64_data = NULL;

  TRACE_EXIT(1);
}

struct exportTensorProtoFile_buffer {
  ProtobufCBuffer base;
  FILE *fd;
};

static void exportTensorProtoFile_append(ProtobufCBuffer *buffer,
            size_t len,
            const uint8_t *data) {
  struct exportTensorProtoFile_buffer *file_buf = (struct exportTensorProtoFile_buffer *) buffer;
  fwrite(data, len, 1, file_buf->fd);
}

size_t exportTensorProtoFile(const Onnx__TensorProto *tensor, char *fname) {

  TRACE_ENTRY(1);

  if (!tensor) {
    TRACE_ERROR(1, true, "tensor is NULL and can not be exported");
    TRACE_EXIT(1);
    return 0;
  }

  TRACE(1, true, "exporting tensor '%s' to file %s", tensor->name, fname);

  struct exportTensorProtoFile_buffer buffer;
  buffer.base.append = &exportTensorProtoFile_append;
  buffer.fd = fopen(fname,"wb");

  size_t size =  onnx__tensor_proto__pack_to_buffer(tensor, (ProtobufCBuffer*) &buffer);

  fclose(buffer.fd);

  TRACE_EXIT(1);
  return size;
}


size_t
strnlen(const char *str, size_t length) {
  size_t count;
  for(count = 0; count < length && str[count]; count++);
  return count;
}

char*
strdup( const char *src ) {
  size_t len = strlen(src);
  char *buffer = malloc(sizeof(char)*(len+1));
  if(!buffer) return NULL;
  strcpy(buffer,src);
  return buffer;
}

char*
strndup( const char *src, size_t num) {
  size_t len = strnlen(src, num);
  char *buffer = malloc(sizeof(char)*(len+1));
  if(!buffer) return NULL;
  strncpy(buffer,src, len);
  buffer[len] = '\0';
  return buffer;
}

void*
memdup( const void *src, size_t size) {
  void *dst = malloc(size);
  if (!dst) return NULL;
  memcpy(dst,src,size);
  return dst;
}

void*
mallocTensorData(Onnx__TensorProto *dst) {
  //TODO check how the datatypes are packed inside the protobuf
  //current assumption: data is densely packed
  size_t  *n_data;
  void   **data;
  size_t   size_element;
  size_t   size_container;
  switch (dst->data_type) {
    case ONNX__TENSOR_PROTO__DATA_TYPE__FLOAT:
      n_data         = &dst->n_float_data;
      data           = (void*) &dst->float_data;
      size_element   = sizeof(float);
      size_container = sizeof(float);
      break;
    case ONNX__TENSOR_PROTO__DATA_TYPE__UINT8:
      n_data         = &dst->n_int32_data;
      data           = (void*) &dst->int32_data;
      size_element   = sizeof(uint8_t);
      size_container = sizeof(int32_t);
      break;
    case ONNX__TENSOR_PROTO__DATA_TYPE__INT8:
      n_data         = &dst->n_int32_data;
      data           = (void*) &dst->int32_data;
      size_element   = sizeof(int8_t);
      size_container = sizeof(int32_t);
      break;
    case ONNX__TENSOR_PROTO__DATA_TYPE__UINT16:
      n_data         = &dst->n_int32_data;
      data           = (void*) &dst->int32_data;
      size_element   = sizeof(uint16_t);
      size_container = sizeof(int32_t);
      break;
    case ONNX__TENSOR_PROTO__DATA_TYPE__INT16:
      n_data         = &dst->n_int32_data;
      data           = (void*) &dst->int32_data;
      size_element   = sizeof(int16_t);
      size_container = sizeof(int32_t);
      break;
    case ONNX__TENSOR_PROTO__DATA_TYPE__INT32:
      n_data         = &dst->n_int32_data;
      data           = (void*) &dst->int32_data;
      size_element   = sizeof(int32_t);
      size_container = sizeof(int32_t);
      break;
    case ONNX__TENSOR_PROTO__DATA_TYPE__INT64:
      n_data         = &dst->n_int64_data;
      data           = (void*) &dst->int64_data;
      size_element   = sizeof(int64_t);
      size_container = sizeof(int64_t);
      break;
    case ONNX__TENSOR_PROTO__DATA_TYPE__STRING:
      n_data         = &dst->n_string_data;
      data           = (void*) &dst->string_data;
      size_element   = sizeof(ProtobufCBinaryData);
      size_container = sizeof(ProtobufCBinaryData);
      break;
    case ONNX__TENSOR_PROTO__DATA_TYPE__BOOL:
      n_data         = &dst->n_int32_data;
      data           = (void*) &dst->int32_data;
      size_element   = sizeof(bool);
      size_container = sizeof(int32_t);
      break;
    case ONNX__TENSOR_PROTO__DATA_TYPE__FLOAT16:
      n_data         = &dst->n_int32_data;
      data           = (void*) &dst->int32_data;
      size_element   = sizeof(float)/2;
      size_container = sizeof(int32_t);
      break;
    case ONNX__TENSOR_PROTO__DATA_TYPE__DOUBLE:
      n_data         = &dst->n_double_data;
      data           = (void*) &dst->double_data;
      size_element   = sizeof(double);
      size_container = sizeof(double);
      break;
    case ONNX__TENSOR_PROTO__DATA_TYPE__UINT32:
      n_data         = &dst->n_uint64_data;
      data           = (void*) &dst->uint64_data;
      size_element   = sizeof(uint32_t);
      size_container = sizeof(uint64_t);
      break;
    case ONNX__TENSOR_PROTO__DATA_TYPE__UINT64:
      n_data         = &dst->n_uint64_data;
      data           = (void*) &dst->uint64_data;
      size_element   = sizeof(uint64_t);
      size_container = sizeof(uint64_t);
      break;
    case ONNX__TENSOR_PROTO__DATA_TYPE__COMPLEX64:
      n_data         = &dst->n_float_data;
      data           = (void*) &dst->float_data;
      size_element   = sizeof(float)*2;
      size_container = sizeof(float);
      break;
    case ONNX__TENSOR_PROTO__DATA_TYPE__COMPLEX128:
      n_data         = &dst->n_double_data;
      data           = (void*) &dst->double_data;
      size_element   = sizeof(double)*2;
      size_container = sizeof(double);
      break;
    case ONNX__TENSOR_PROTO__DATA_TYPE__BFLOAT16:
      //is not documented
    default:
      //unknown datatype
      TRACE_FATAL(0, true, "no case for datatype %d", dst->data_type);
      return NULL;
  }
  size_t num = dst->dims[0];
  for (int i = 1; i < dst->n_dims; i++) {
    num *= dst->dims[i];
  }
  *n_data = (num*size_element+size_container-1)/size_container;
  *data   = malloc( (*n_data) * size_container);

  return *data;
}

void*
freeTensorData(Onnx__TensorProto *dst) {
  void   **data;
  switch (dst->data_type) {
    case ONNX__TENSOR_PROTO__DATA_TYPE__FLOAT:
      data           = (void*) &dst->float_data;
      break;
    case ONNX__TENSOR_PROTO__DATA_TYPE__UINT8:
      data           = (void*) &dst->int32_data;
      break;
    case ONNX__TENSOR_PROTO__DATA_TYPE__INT8:
      data           = (void*) &dst->int32_data;
      break;
    case ONNX__TENSOR_PROTO__DATA_TYPE__UINT16:
      data           = (void*) &dst->int32_data;
      break;
    case ONNX__TENSOR_PROTO__DATA_TYPE__INT16:
      data           = (void*) &dst->int32_data;
      break;
    case ONNX__TENSOR_PROTO__DATA_TYPE__INT32:
      data           = (void*) &dst->int32_data;
      break;
    case ONNX__TENSOR_PROTO__DATA_TYPE__INT64:
      data           = (void*) &dst->int64_data;
      break;
    case ONNX__TENSOR_PROTO__DATA_TYPE__STRING:
      data           = (void*) &dst->string_data;
      break;
    case ONNX__TENSOR_PROTO__DATA_TYPE__BOOL:
      data           = (void*) &dst->int32_data;
      break;
    case ONNX__TENSOR_PROTO__DATA_TYPE__FLOAT16:
      data           = (void*) &dst->int32_data;
      break;
    case ONNX__TENSOR_PROTO__DATA_TYPE__DOUBLE:
      data           = (void*) &dst->double_data;
      break;
    case ONNX__TENSOR_PROTO__DATA_TYPE__UINT32:
      data           = (void*) &dst->uint64_data;
      break;
    case ONNX__TENSOR_PROTO__DATA_TYPE__UINT64:
      data           = (void*) &dst->uint64_data;
      break;
    case ONNX__TENSOR_PROTO__DATA_TYPE__COMPLEX64:
      data           = (void*) &dst->float_data;
      break;
    case ONNX__TENSOR_PROTO__DATA_TYPE__COMPLEX128:
      data           = (void*) &dst->double_data;
      break;
    case ONNX__TENSOR_PROTO__DATA_TYPE__BFLOAT16:
      //is not documented
    default:
      //unknown datatype
      TRACE_FATAL(0, true, "no case for datatype %d", dst->data_type);
      return NULL;
  }
  free(*data);

  return *data;
}
