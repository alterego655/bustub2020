//===----------------------------------------------------------------------===//
//
//                         BusTub
//
// insert_executor.cpp
//
// Identification: src/execution/insert_executor.cpp
//
// Copyright (c) 2015-19, Carnegie Mellon University Database Group
//
//===----------------------------------------------------------------------===//
#include <memory>

#include "execution/executors/insert_executor.h"

namespace bustub {

InsertExecutor::InsertExecutor(ExecutorContext *exec_ctx, const InsertPlanNode *plan,
                               std::unique_ptr<AbstractExecutor> &&child_executor)
    : AbstractExecutor(exec_ctx), plan_(plan), child_executor_ptr_(std::move(child_executor)) {}

void InsertExecutor::Init() {
  Catalog *catalog = GetExecutorContext()->GetCatalog();
  table_metadata_ptr_ = catalog->GetTable(plan_->TableOid());
  const std::string &table_name = table_metadata_ptr_->name_;
  index_info_vector_ = catalog->GetTableIndexes(table_name);  // Will there be copy elision(NRVO)?
  if (plan_->IsRawInsert()){
    iter_ = plan_->RawValues().begin();
  }
  if (!plan_->IsRawInsert()){
      child_executor_ptr_->Init();
  }
}

void InsertExecutor::InsertTuple(Tuple &tuple, RID *rid){
  TableHeap *table_heap_ptr = table_metadata_ptr_->table_.get();
  table_heap_ptr->InsertTuple(tuple, rid, GetExecutorContext()->GetTransaction());
  for (auto &index_info:index_info_vector_){
    B_PLUS_TREE_INDEX_TYPE *b_plus_tree_index
        = reinterpret_cast<B_PLUS_TREE_INDEX_TYPE*>(index_info->index_.get());
    b_plus_tree_index->InsertEntry(tuple.KeyFromTuple(table_metadata_ptr_->schema_,
                                                             index_info->key_schema_,
                                                             index_info->index_->GetMetadata()->GetKeyAttrs()),
                                   *rid,GetExecutorContext()->GetTransaction());
  }
}

bool InsertExecutor::Next([[maybe_unused]] Tuple *tuple, RID *rid) {
  if (!plan_->IsRawInsert()){
    if (child_executor_ptr_->Next(tuple, rid)){
      InsertTuple(*tuple, rid);
      return true;
    }
    return false;
  }

  if (iter_ != plan_->RawValues().end()){
    Tuple insert_tuple(*iter_++, &table_metadata_ptr_->schema_);
    InsertTuple(insert_tuple, rid);
    return true;
  }
  return false;
}

}  // namespace bustub
