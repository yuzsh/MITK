/*=========================================================================
 
Program:   Medical Imaging & Interaction Toolkit
Module:    $RCSfile$
Language:  C++
Date:      $Date$
Version:   $Revision$
 
Copyright (c) German Cancer Research Center, Division of Medical and
Biological Informatics. All rights reserved.
See MITKCopyright.txt or http://www.mitk.org/copyright.html for details.
 
This software is distributed WITHOUT ANY WARRANTY; without even
the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
PURPOSE.  See the above copyright notices for more information.
 
=========================================================================*/


#ifndef _CommonFunctionality__h_
#define _CommonFunctionality__h_

#include "mitkConfig.h"

// std
#include <string>
#include <vector>

#include <ipPic/ipPic.h>

// itk includes
#include <itkMinimumMaximumImageCalculator.h>

// mitk includes
#include <mitkProperties.h>
#include <mitkLevelWindowProperty.h>
#include <mitkStringProperty.h>
#include <mitkDataTreeNode.h>
#include <mitkDataTreeNodeFactory.h>
// #include <mitkImageCast.h>
#include <mitkDataTree.h>
#include <mitkSurface.h>
#include "itkImage.h"

#include <qstring.h>
#include <qfiledialog.h>

#include <itksys/SystemTools.hxx>

#include "mitkLevelWindow.h"

namespace mitk
{
  class Surface;
  class Image;
}

class vtkPolyData;

#ifdef MBI_INTERNAL
#ifndef QMITKCOMMONFUNCTIONALITYIMPLEMENTATION
//#include <boost/graph/graph_traits.hpp>
//#include <boost/graph/graph_selectors.hpp>
namespace boost
{
  struct directedS;
  struct undirectedS;
}
namespace mitk
{
  template <typename DirectedCategory> class VesselGraphData;
  typedef VesselGraphData< boost::undirectedS  > UndirectedVesselGraphData;
  typedef VesselGraphData< boost::directedS > DirectedVesselGraphData;
}
#endif
#endif

/**
 * This class provides some data handling methods, like loading data or adding different 
 * types to the data tree...
 */
namespace CommonFunctionality
{

  const char* GetInternalFileExtensions();
  const char* GetExternalFileExtensions();
  const char* GetSaveFileExtensions();

  typedef std::vector<mitk::DataTreeIteratorClone> DataTreeIteratorVector;
  typedef std::vector<mitk::DataTreeNode*> DataTreeNodeVector;
  /** \brief compute min and max
  */
  template < typename TImageType >
  static void MinMax(typename TImageType::Pointer image, float &min, float &max)
  {
    typedef itk::MinimumMaximumImageCalculator<TImageType> MinMaxCalcType;
    typename MinMaxCalcType::Pointer minmax = MinMaxCalcType::New();
    minmax->SetImage( image );
    minmax->ComputeMinimum();
    minmax->ComputeMaximum();

    min = (float) minmax->GetMinimum();
    max = (float) minmax->GetMaximum();


  }

  template < typename TImageType >
  static void AutoLevelWindow( mitk::DataTreeNode::Pointer node )
  {
    mitk::Image::Pointer image = dynamic_cast<mitk::Image*> (node->GetData() );
    if ( image.IsNotNull() )
    {
      typename TImageType::Pointer itkImage= TImageType::New();
      mitk::CastToItkImage( image, itkImage);
      float extrema[2];
      extrema[0] = 0;
      extrema[1] = 4096;
      MinMax<TImageType>(itkImage,extrema[0],extrema[1]);

      mitk::LevelWindowProperty::Pointer levWinProp = dynamic_cast<mitk::LevelWindowProperty*>(node->GetPropertyList()->GetProperty("levelwindow").GetPointer());
      if( levWinProp.IsNull() )
      {
        levWinProp = new mitk::LevelWindowProperty();
        node->GetPropertyList()->SetProperty("levelwindow", levWinProp);
      }

      double window = (extrema[1] - extrema[0])/10.0;
      double level  = window/2;

      mitk::LevelWindow levWin = levWinProp->GetLevelWindow();
      levWin.SetRangeMin(extrema[0]);
      levWin.SetRangeMax(extrema[1]);
      levWin.SetLevel( level );
      levWin.SetWindow( window );
      levWinProp->SetLevelWindow(levWin);

    }
  }


  /**
  * \brief converts the itk image to mitk image, creates a datatreenode and adds the node to 
  * the referenced datatree
  */
  template < typename TImageType >
  static mitk::DataTreeNode::Pointer
  AddItkImageToDataTree(typename TImageType::Pointer itkImage, mitk::DataTreeIteratorBase* iterator, std::string str)
  {
    mitk::DataTreeIteratorClone it=iterator;

    mitk::Image::Pointer image = mitk::Image::New();
    image->InitializeByItk(itkImage.GetPointer());
    image->SetVolume(itkImage->GetBufferPointer());

    mitk::DataTreeNode::Pointer node = NULL;
    mitk::DataTreeIteratorClone subTree = ((mitk::DataTree *) it->GetTree())->GetNext("name", new mitk::StringProperty( str.c_str() ));

    if (subTree->IsAtEnd() || subTree->Get().IsNull() )
    {
      node=mitk::DataTreeNode::New();
      mitk::StringProperty::Pointer nameProp = new mitk::StringProperty(str.c_str());
      node->SetProperty("name",nameProp);
      node->SetData(image);
      it->Add(node);
    }
    else
    {
      node = subTree->Get();
      node->SetData(image);
    }

    mitk::LevelWindowProperty::Pointer levWinProp = new mitk::LevelWindowProperty();
    mitk::LevelWindow levelWindow;
    node->GetPropertyList()->SetProperty("levelwindow",levWinProp);
    node->SetProperty("volumerendering",new mitk::BoolProperty(false));

    float extrema[2];
    extrema[0] = 0;
    extrema[1] = 4096;

    CommonFunctionality::MinMax<TImageType>(itkImage,extrema[0],extrema[1]);
    if (extrema[0] == 0 && extrema[1] ==1)
    {
      mitk::BoolProperty::Pointer binProp = new mitk::BoolProperty(true);
      node->GetPropertyList()->SetProperty("binary",binProp);
    }
    return node;
  }



  /**
  * \brief converts the itk image to mitk image, creates a datatreenode and adds the node to 
  * the referenced datatree
  */
  template < typename TMeshType >
  static void AddItkMeshToDataTree(typename TMeshType::Pointer itkMesh, mitk::DataTreeIteratorBase* iterator, std::string str)
  {
    mitk::DataTreeIteratorClone it=iterator;

    mitk::DataTreeNode::Pointer node = NULL;
    mitk::DataTreeIteratorClone subTree = ((mitk::DataTree *) it->GetTree())->GetNext("name", new mitk::StringProperty( str.c_str() ));

    if (subTree->IsAtEnd() || subTree->Get() == NULL )
    {
      node=mitk::DataTreeNode::New();
      mitk::StringProperty::Pointer nameProp = new mitk::StringProperty(str.c_str());
      node->SetProperty("name",nameProp);
      it->Add(node);
    }
    else
    {
      node = subTree->Get();
    }

    mitk::Surface::Pointer surface = mitk::Surface::New();
    //    vtkPolyData* polys = MeshUtil<TMeshType>::meshToPolyData( itkMesh );
    /**
    * @todo include Algorithms/itkMeshDeformation into Framework module so the upper line can be used
    * and the conversion works correctly
    */
    vtkPolyData* polys = vtkPolyData::New();
    surface->SetVtkPolyData(polys);
    node->SetData( surface );
    node->SetProperty("layer", new mitk::IntProperty(1));
    node->SetVisibility(true,NULL);

    float meshColor[3] = {.5f,.5f,.5f};
    node->SetColor(meshColor,  NULL );
    node->SetVisibility(true, NULL );
  }
  /**
  * \brief converts the itk image to mitk image, creates a datatreenode and adds the node to 
  * the referenced datatree
  */
  mitk::DataTreeNode::Pointer AddVtkMeshToDataTree(vtkPolyData* polys, mitk::DataTreeIteratorBase* iterator, std::string str);

  /**
  * \brief creates a datatreenode for th PIC image and adds the node to 
  * the referenced datatree
  */
  mitk::DataTreeNode::Pointer AddPicImageToDataTree(ipPicDescriptor * pic, mitk::DataTreeIteratorBase* iterator, std::string str);

  void SetWidgetPlanesEnabled(mitk::DataTree* dataTree, bool enable);
  mitk::DataTreeNode::Pointer FileOpen( const char * fileName );
  mitk::DataTreeNode::Pointer FileOpenImageSequence(const char* fileName);
  mitk::DataTreeNode::Pointer FileOpenImageSequence();
  mitk::DataTreeNode::Pointer FileOpen();
  mitk::DataTreeNode::Pointer OpenVolumeOrSliceStack();

  std::string SaveImage(mitk::Image* image, const char* fileName = NULL);

  std::string SaveSurface(mitk::Surface* surface, const char* fileName = NULL);

#ifdef MBI_INTERNAL
  void SaveDirectedVesselGraph( mitk::DirectedVesselGraphData* graph, const char* fileName = NULL );
  void SaveUndirectedVesselGraph( mitk::UndirectedVesselGraphData* graph, const char* fileName = NULL );
#endif

  void SaveBaseData( mitk::BaseData* data, const char* name = NULL );



  static mitk::DataTreeIteratorBase* GetIteratorToFirstImage(mitk::DataTreeIteratorBase* dataTreeIterator)
  {
    mitk::DataTreeIteratorClone it = dataTreeIterator;
    while ( !it->IsAtEnd() )
    {
      mitk::DataTreeNode::Pointer node = it->Get();
      if ( node->GetData() != NULL )
      {
        // access the original image
        mitk::Image::Pointer img = dynamic_cast<mitk::Image*>( node->GetData() );

        // enquiry whether img is NULL
        if ( img.IsNotNull() )
        {
          return it->Clone();
        }
      }
      ++it;
    }
    std::cout << "No node containing an mitk::Image found, returning NULL..." << std::endl;
    return NULL;
  }

  mitk::DataTreeIteratorBase* GetIteratorToFirstImageInDataTree(mitk::DataTree::Pointer dataTree);
  mitk::Image* GetFirstImageInDataTree(mitk::DataTree::Pointer dataTree);

  /**
   * Searches for the first node in the data tree, which holds a given type. 
   * @param it an iterator pointing to the position in the data tree, where
   *          the search should start
   * @returns the first node in the data tree which is of the type given by
   *          the template parameter T, or NULL otherwise.
   */
  template <typename T>
  static mitk::DataTreeNode* GetFirstNodeByType( mitk::DataTreeIteratorClone it )
  {
    if ( it.GetPointer() == NULL )
    {
      return NULL;
    }

    mitk::DataTreeIteratorClone iteratorClone = it;
    while ( !iteratorClone->IsAtEnd() )
    {
      mitk::DataTreeNode::Pointer node = iteratorClone->Get();
      if ( node->GetData() != NULL )
      {
        // access the original data
        T* data = dynamic_cast<T*>( node->GetData() );

        // enquiry whether img is NULL
        if ( data != NULL )
        {
          return node.GetPointer();
        }
      }
      ++iteratorClone;
    }
    return NULL;
  }

  /**
   * Searches for the data object in the data tree, which matches a given type. 
   * @param it an iterator pointing to the position in the data tree, where
   *          the search should start
   * @returns the first data oobject in the data tree which is of the type given by
   *          the template parameter T, or NULL otherwise.
   */
  template <typename T>
  static T* GetFirstDataByType( mitk::DataTreeIteratorClone it )
  {
    mitk::DataTreeNode* node = GetFirstNodeByType<T>(it);
    if ( node == NULL )
    {
      return NULL;
    }
    else
    {
      return dynamic_cast<T*>( node->GetData() );
    }
  }

  /**
   * Searches for the first node in the data tree, which holds a given type 
   * and matches a given property key and value. This may be used to search
   * e.g. for a node holding an image with a given name in the data tree.
   * @param it an iterator pointing to the position in the data tree, where
   *          the search should start
   * @param propertyKey the name of the property we want to compare with
   * @param property the value of the property we want to search for in the data tree
   * @returns the first node in the data tree which is of the type given by
   *          the template parameter T, and matches propertyKey and property, or NULL otherwise.
   */
  template <typename T>
  static mitk::DataTreeNode* GetFirstNodeByTypeAndProperty( mitk::DataTreeIteratorClone it, std::string propertyKey, mitk::BaseProperty* property )
  {
    if ( it.GetPointer() == NULL )
    {
      return NULL;
    }

    mitk::DataTreeIteratorClone iteratorClone = it;
    while ( !iteratorClone->IsAtEnd() )
    {
      mitk::DataTreeNode::Pointer node = iteratorClone->Get();
      if ( node->GetData() != NULL )
      {
        // access the original data
        T* data = dynamic_cast<T*>( node->GetData() );

        // enquiry whether img is NULL
        if ( data != NULL )
        {
          // check, if the data has the given property...
          mitk::BaseProperty::Pointer tmp = node->GetPropertyList()->GetProperty( propertyKey.c_str() );
          if ( (*property) == *(tmp) )
            return node.GetPointer();
        }
      }
      ++iteratorClone;
    }
    return NULL;
  }

  /**
   * Searches for the first data object in the data tree, which is of a given type 
   * and whose node matches a given property key and value. This may be used to search
   * e.g. for an image with a given name in the data tree.
   * @param it an iterator pointing to the position in the data tree, where
   *          the search should start
   * @param propertyKey the name of the property we want to compare with
   * @param property the value of the property we want to search for in the data tree
   * @returns the first data object in the data tree which is of the type given by
   *          the template parameter T, and matches propertyKey and property, or NULL otherwise.
   */
  template <typename T>
  static T* GetFirstDataByTypeAndProperty( mitk::DataTreeIteratorClone it, std::string propertyKey, mitk::BaseProperty* property )
  {
    mitk::DataTreeNode* node = GetFirstNodeByTypeAndProperty<T>( it, propertyKey, property );
    if ( node == NULL )
    {
      return NULL;
    }
    else
    {
      return dynamic_cast<T*>( node->GetData() );
    }
  }

  /**
   * Searches for the first node in the data tree, which matches a given 
   * property key and value. This may be used to search e.g. for a node holding
   * a data object with a given name in the data tree.
   * @param it an iterator pointing to the position in the data tree, where
   *          the search should start
   * @param propertyKey the name of the property we want to compare with
   * @param property the value of the property we want to search for in the data tree
   * @returns the first node in the data tree which matches propertyKey and property, or NULL otherwise.
   */
  mitk::DataTreeNode* GetFirstNodeByProperty( mitk::DataTreeIteratorClone it, std::string propertyKey, mitk::BaseProperty* property );

  /**
   * Searches for the first data object in the data tree, whose node matches a given 
   * property key and value. This may be used to search e.g. for a node holding
   * a data object with a given name in the data tree.
   * @param it an iterator pointing to the position in the data tree, where
   *          the search should start
   * @param propertyKey the name of the property we want to compare with
   * @param property the value of the property we want to search for in the data tree
   * @returns the first data object in the data tree whose node matches propertyKey and property, or NULL otherwise.
   */
  mitk::BaseData* GetFirstDataByProperty( mitk::DataTreeIteratorClone it, std::string propertyKey, mitk::BaseProperty* property );
  /**
   * Searches for the node in the data tree which holds a given mitk::BaseData 
   * @param it an iterator pointing to the position in the data tree, where
   *          the search should start
   * @param data the data object, for which the node in the tree should be searched.
   * @returns the node holding data, or NULL otherwise.
   */
  mitk::DataTreeNode* GetNodeForData( mitk::DataTreeIteratorClone it, mitk::BaseData* data );

  template <typename BaseDataType>
  static DataTreeNodeVector GetNodesForDataType(mitk::DataTreeIteratorClone it)
  {
    DataTreeNodeVector result;

    if ( it.GetPointer() != NULL )
    {

      mitk::DataTreeIteratorClone iteratorClone = it->Clone();
      while ( !iteratorClone->IsAtEnd() )
      {
        mitk::DataTreeNode::Pointer node = iteratorClone->Get();
        if ( dynamic_cast<BaseDataType*>( node->GetData() ) )
        {
          result.push_back(node);
        }
        ++iteratorClone;
      }
    }
    return result;

  }

  DataTreeIteratorVector FilterNodes(mitk::DataTreeIteratorClone it, bool (* FilterFunction)(mitk::DataTreeNode*));
};
#endif // _CommonFunctionality__h_
