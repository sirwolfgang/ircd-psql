//==============================================================================
// File:	db_sql.cpp
// Purpose: Provide bi-directional commication based on sql store
//==============================================================================
#include "db_sql.h"

//------------------------------------------------------------------------------
bool DBSQL::isConnectionReady()
{
  if (m_isDatabaseLoaded && m_hDatabaseConnection)
  {
    if (Anope::ReadOnly)
    {
      Anope::ReadOnly = false;
      Log() << "Database Ready";
    }

    return true;
  }
  else
  {
    Log() << "Failed to connect to database - Read Only Mode Active";
    Anope::ReadOnly = true;
    
    return false;
  }
}

//------------------------------------------------------------------------------
DBSQL::DBSQL(const Anope::string& _modname, const Anope::string& _creator)
  : Module(_modname, _creator, DATABASE | VENDOR),
  m_hDatabaseConnection("", ""),
  m_isDatabaseLoaded(false)
{
  if (ModuleManager::FindFirstOf(DATABASE) != this)
    throw ModuleException("If db_sql is loaded it must be the first database module loaded.");
}

//------------------------------------------------------------------------------
void DBSQL::OnNotify() anope_override
{
  if (!this->isConnectionReady())
    return;
  
  Log(LOG_DEBUG) << "DBSQL::OnNotify";

  for (std::map<Serializable*, EACTION>::iterator it = m_changeList.begin(); it != m_changeList.end(); ++it)
  {
    Serializable* _pObject = it->first;
    EACTION action = it->second;
    
    switch(action)
    {
    case CREATE:
      m_hDatabaseConnection->Create(_pObject);
      break;
      
    case UPDATE:
      m_hDatabaseConnection->Update(_pObject);
      break;
    }
    
    _pObject->UpdateTS();
  }
  
  m_changeList.clear();

//     if (pObject->IsCached(data))
//       continue;

//     pObject->UpdateCache(data);

//     Serialize::Type *s_type = pObject->GetSerializableType();

//     if (!s_type)
//       continue;

//     if (res.GetID() && pObject->id != res.GetID())
//     {
//       /* In this case object is new, so place it into the object map */
//       pObject->id = res.GetID();
//       s_type->objects[pObject->id] = pObject;
//     }
//   }

}

//------------------------------------------------------------------------------
EventReturn DBSQL::OnLoadDatabase() anope_override
{
  m_isDatabaseLoaded = true;
  return EVENT_STOP;
}

//------------------------------------------------------------------------------
void DBSQL::OnShutdown() anope_override
{
  m_isDatabaseLoaded = false;
}

//------------------------------------------------------------------------------
void DBSQL::OnRestart() anope_override
{
  m_isDatabaseLoaded = false;
}

//------------------------------------------------------------------------------
void DBSQL::OnReload(Configuration::Conf* _pConfig) anope_override
{
  Configuration::Block* pBlock = _pConfig->GetModule(this);
  m_hDatabaseConnection = ServiceReference<Datastore::Provider>("Datastore::Provider", pBlock->Get<const Anope::string>("engine"));
}

//------------------------------------------------------------------------------
void DBSQL::OnSerializableConstruct(Serializable* _pObject) anope_override
{
  if (!this->isConnectionReady())
    return;

  m_changeList.insert(std::pair<Serializable*, EACTION>(_pObject, CREATE));
  Notify();
}

//------------------------------------------------------------------------------
void DBSQL::OnSerializeCheck(Serialize::Type* _pObject) anope_override
{
  if (!this->isConnectionReady())
    return;
  
  m_hDatabaseConnection->Read(NULL);
  //_pObject->UpdateTS();
}

//------------------------------------------------------------------------------
void DBSQL::OnSerializableUpdate(Serializable* _pObject) anope_override
{
  if (!this->isConnectionReady())
    return;
  
  m_changeList.insert(std::pair<Serializable*, EACTION>(_pObject, UPDATE));
  Notify();
}

//------------------------------------------------------------------------------
void DBSQL::OnSerializableDestruct(Serializable* _pObject) anope_override
{
  if (!this->isConnectionReady())
    return;

  m_changeList.erase(_pObject);
  m_hDatabaseConnection->Destroy(_pObject);
  
  _pObject->GetSerializableType()->objects.erase(_pObject->id);
}

