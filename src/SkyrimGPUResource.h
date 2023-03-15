#pragma once
#include "d3d11.h"
class SkyrimGPUResource
{
public:
	/// Resource behavior type
	enum Behavior
	{
		GPU_RESOURCE_STATIC,    ///< This resource's contents are never modified after creation
		GPU_RESOURCE_DYNAMIC,   ///< This resource will be used for rendering into and will be modifiable at run-time
		GPU_RESOURCE_UPLOAD,    ///< This resource is written by CPU for GPU to read once or copy from
		GPU_RESOURCE_READBACK,  ///< This resource is written once or copied into by GPU and read on CPU

	}; // End of enum Behavior declaration

	/// Resource type 
	enum Type
	{
		GPU_RESOURCE_UNKNOWN = 0,        ///< Unknown resource type, reserved
		GPU_RESOURCE_BUFFER,             ///< Buffer resource type, can be used for data buffer, vertex buffer or index buffer
		GPU_RESOURCE_TEXTURE_2D_ARRAY,   ///< Texture resource type, default array dimension is 1 slice for texture images
		GPU_RESOURCE_TEXTURE_CUBE_ARRAY, ///< Texture cube resource type, default array dimension is 1 slice for std. cube maps
		GPU_RESOURCE_TEXTURE_3D          ///< Volumetric texture resource type

	}; // End of enum Type declaration

	/// Describes how which pipeline stages this resource can be bound to. Note that these enums can be 
	/// bitwise ORed together.
	enum BindType
	{
		BIND_VERTEX_BUFFER = 0x1L,       ///< Resource can be used as a vertex buffer
		BIND_INDEX_BUFFER = 0x2L,        ///< Resource can be used as an index buffer
		BIND_CONSTANT_BUFFER = 0x4L,     ///< Resource can be used as a constant buffer
		BIND_SHADER_RESOURCE = 0x8L,     ///< Resource can be used as a shader resource
		BIND_STREAM_OUTPUT = 0x10L,      ///< Resource can be used as a streaming output buffer
		BIND_RENDER_TARGET = 0x20L,      ///< Resource can be used as a rendertarget
		BIND_DEPTH_STENCIL = 0x40L,      ///< Resource can be used as a depth stencil
		BIND_UNORDERED_ACCESS = 0x80L    ///< Resource can be used as a UAV
	}; // End of enum BindType declaration

	/// State of a resource, which includes view states as well as copy source or destination states.
	/// For use with Transition() to serialize two types of GPU access.
	enum StateType
	{
		STATE_COMMON = 0,                 ///< Generic initial state
		STATE_VERTEX_BUFFER,              ///< Resource is read/written as a vertex buffer
		STATE_INDEX_BUFFER,               ///< Resource is read/written as an index buffer
		STATE_CONSTANT_BUFFER,            ///< Resource is read/written as a constant buffer
		STATE_PIXEL_SHADER_RESOURCE,      ///< Resource is read as a shader resource in a pixel shader
		STATE_NON_PIXEL_SHADER_RESOURCE,  ///< Resource is read as a shader resource in a non-pixel shader
		STATE_DEPTH_STENCIL_READ,         ///< Resource is read as a depth stencil
		STATE_INDIRECT_ARGUMENT,          ///< Resource is read as an indirect argument
		STATE_GENERIC_READ,               ///< Resource is used in a read
		STATE_COPY_SOURCE,                ///< Resource is read as the source of a copy
		STATE_PRESENT,                    ///< Resource is presented
		STATE_MULTISAMPLE_RESOLVE_SOURCE, ///< Resource is source of multisample subresource resolve
		STATE_MULTISAMPLE_RESOLVE_DEST,   ///< Resource is destination of multisample subresource resolve
		STATE_STREAM_OUTPUT,              ///< Resource is read/written as a streaming output buffer
		STATE_RENDER_TARGET,              ///< Resource is written as a rendertarget
		STATE_DEPTH_STENCIL_WRITE,        ///< Resource is written as a depth stencil
		STATE_UNORDERED_ACCESS,           ///< Resource is read/written as a UAV
		STATE_COPY_DEST,                  ///< Resource is written as a copy destination
	}; // End of enum StateType declaration

	/// Misc creation flags for hinting to driver about the usage of this resource
	enum MiscHint
	{
		MISC_HINT_NONE = 0x0L, ///< No hints
		MISC_HINT_SHARED_RESOURCE = 0x1L, ///< Enables the sharing of resource data between two or more devices
		MISC_HINT_DRAWINDIRECT_ARGS = 0x2L  ///< Buffer can be used as argument for indirect draws or dispatches
	}; // End of enum MiscHint

	/// Describes how this resource will be accessed by the CPU (write / read). Note that these flags can 
	/// be ORed. Also note that for certain resource behavior / type combinations only read-access is 
	/// allowed for CPU access and will be enforced.  CPU_ACCESS_PERFORMANT_READ is a hint that the resource will be
	/// read frequently, and that resource implementations should attempt to provide fast readback
	///  (for example, by caching any staging resources that are needed for the readback)
	enum CPUAccessType
	{
		CPU_ACCESS_WRITE = 0X10000L, ///< CPU has write access
		CPU_ACCESS_READ = 0x20000L, ///< CPU has read access
		CPU_ACCESS_PERFORMANT_READ = 0x40000L  ///< CPU has read access, and we would like it to be fast.

	};  // End of enum CPUAccessType declaration

	/// Options for read and write permissions for a mapped resource for access. Note that the resource
	/// must be first created with appropriate flags in order to use the right options.
	enum MapType
	{
		MAP_READ,                        ///< Map a resource and make it readable. Only allowed to be used with CPU_ACCESS_READ 
		///  specified resources.
		MAP_WRITE,                       ///< Map a resource and make it writeable. Can only be used if the resource is created
		///  with CPU_ACCESS_WRITE. Used for only overwriting parts of the buffer.
		MAP_READ_WRITE,                  ///< Map a resource and make it readable and writable. Both CPU_ACCESS_READ and 
		///  CPU_ACCESS_WRITE must be used to create resource in order to use this option
		MAP_WRITE_DISCARD,               ///< Map a resource, discard its contents and make it writable. Used for overwriting 
		///  the whole buffer. CPU_ACCESS_WRITE must be used during resource creation in order 
		///  to use this option
		MAP_WRITE_NO_OVERWRITE,          ///< Map a resource and make it writable.  The app promises to not overwrite any 
		///  previously written portions of the resource.  Only valid on resources created with 
		///  CPU_ACCESS_WRITE.  

	}; // End of enum MapType declaration
	/// Default constructor
	SkyrimGPUResource();

	/// Constructor
	SkyrimGPUResource(Behavior eBehavior, Type eType, uint32_t nBindFlags, uint32_t nCPUAccessFlags, uint32_t nMiscHintFlags);

	/// Destructor
	virtual ~SkyrimGPUResource();

	/// Get API Handle to a GPU resource
	///
	/// \pre Resource has been successfully allocated
	/// \return An API-specific handle to the resource
	virtual SuHandle GetHandle() const = 0;

	/// Get the behavior type for this resource
	///
	/// \return Current behavior type
	inline const Behavior GetBehavior() const { return m_eBehavior; }

	/// Get the resource type of this resource (texture / buffer / etc)
	///
	/// \return Current resource type
	inline const Type GetType()   const { return m_eType; }

	/// Get bind flags for this resource
	inline const uint32_t GetBindFlags() const { return m_nBindFlags; }

	/// Get CPU access flags for this resource
	inline const uint32_t GetCPUAccessFlags() const { return m_nCPUAccessFlags; }

	/// Get Misc hint flags for this resource
	inline const uint32_t GetMiscHintFlags() const { return m_nMiscHintFlags; }

	/// Returns true if this resource can be used according to a particular bind type option (i.e. usage type)
	bool QueryUsage(BindType eUsageOption) const;

	/// Helper method, allows querying whether a given bind type is found in bind flags
	static bool QueryBindUsage(BindType eUsageOption, uint32 nBindFlags);

	/// Returns true if this resource can be accessed according to particular CPU access option
	bool QueryCPUAccess(CPUAccessType eCPUAccessOption) const;

	/// Query Misc hint flag for the resource 
	bool QueryMiscHint(MiscHint eMiscHint) const;

	/// Unmap (and update) the resource that was previously mapped
	/// Caller responsible for synchronizing with Transition before and after; transition to STATE_COPY_DEST before calling.
	///
	/// \return True if no errors occurred during unmapping, false if trying to unmap a resource that hasn't been created previously
	virtual bool Unmap() = 0;

	/// Ensure completion of start state by GPU before transition to end state
	virtual bool Transition(StateType startState, StateType endState);

	/// Ensure completion of all previous UAV reads or writes, before subsequent UAV reads or writes occur
	virtual bool UAVBarrier();

	/// Copy entire resource
	virtual bool CopyResource(const SkyrimGPUResource& srcResource);

	/// A convenience function which returns a sampling view of the entire resource
	virtual SkyrimGPUSamplingResourceViewPtr GetDefaultSamplingView() = 0;

	/// A convenience function which returns a sampling view description of the entire resource
	virtual SkyrimGPUResourceDescription GetDefaultResourceDesc() = 0;

	/// Returns a sampling view for this resource
	SkyrimGPUSamplingResourceView* GetSamplingView(const SkyrimGPUResourceViewDescription& rDesc);

	/// Returns a renderable view for this resource
	SkyrimGPURenderableResourceView* GetRenderableView(const SkyrimGPUResourceViewDescription& rDesc);

	/// Retrieves a UAV for this resource
	SkyrimGPUUnorderedAccessView* GetUnorderedAccessView(const SkyrimGPUResourceViewDescription& rDesc);

	/// Returns a reference to the array containing the existing views for this object
	inline const SkyrimGPUResourceViewArray& GetExistingViews() const { return m_viewArray; };

	/// Returns the name assigned to this resource.  This is the name that is given when creating resource from scripts.
	inline const std::string& GetName() const { return m_name; };

	//tolua_end

		/// Sets the name assigned to this resource.  This should only be called by the GRM at resource creation time
		/// \param rName  A name to assign to the resource
	inline void SetName(const std::string& rName) { m_name = rName; };

	/// Sets the MD5 hash assigned to this resource.  This should only be called by the GRM at resource creation time
	/// \param pHash  A pointer to an MD5 hash that is associated with this resource.  The resource does NOT assume
	///        ownership of the hash.  The hash is simply stored with the resource for simplicity's sake
	inline void SetHash(const SuMD5Hash* pHash) { m_pHash = pHash; };

	/// Returns the MD5 hash associated with this resource
	inline const SuMD5Hash* GetHash() const { return m_pHash; };

	/// Returns a counter identifying when this resource was created.  As they are created, resources are numbered with a sequential counter
	/// This counter can be used to track down resource leaks by breaking when a particular resource is created
	inline uint32_t GetAllocID() const { return m_nAllocID; };

protected:

	/// Interface for creating an API-specific sampling view
	virtual SkyrimGPUSamplingResourceView* CreateSamplingView(const SkyrimGPUResourceViewDescription& rDesc) = 0;

	/// Interface for creating an API-specific renderable view
	virtual SkyrimGPURenderableResourceView* CreateRenderableView(const SkyrimGPUResourceViewDescription& rDesc) = 0;

	/// Interface for creating an API-SPECIFIC unordered access view
	virtual SkyrimGPUUnorderedAccessView* CreateUnorderedAccessView(const SkyrimGPUResourceViewDescription& rDesc);

	/// Sub-class specific method to determine whether the resource can support a particular view
	virtual bool IsViewValid(const SkyrimGPUResourceViewDescription& rDesc) const;

	/// Locates an existing view on this resource whose description matches the given description
	SkyrimGPUResourceViewPtr FindResourceView(const SkyrimGPUResourceViewDescription& rDesc);

	/// Adds a resource view to this resource
	bool AddResourceView(SkyrimGPUResourceView* pResourceView);

	/// Removes a resource view from this resource
	bool RemoveResourceView(SkyrimGPUResourceView* pResourceView);

	/// Verifies that a particular mapping mode is legal for this resource
	bool ValidateMapType(MapType eMapOption) const;

private:

	/// Helper method that is called by constructors to assign an alloc ID to the resource
	void SetAllocID();



	Behavior       m_eBehavior;       ///< GPU resource usage behavior 

	Type           m_eType;           ///< GPU resource type         

	uint32_t         m_nBindFlags;       ///< Bitwise combination of GPU resource bind flags

	uint32_t         m_nCPUAccessFlags;  ///< Bitwise combination of CPU access specification

	uint32_t         m_nMiscHintFlags;   ///< Bitwise combination of Misc hint flags

	SkyrimGPUResourceViewArray  m_viewArray;   ///< An array of views that this resource uses for rendering

	bool           m_bIsMapped;       ///< This specified whether the resource is being mapped or not

	std::string       m_name;            ///< A globally unique name that can be used to identify this resource (e.g. in app-updates)

	const SuMD5Hash* m_pHash;     ///< An MD5 hash that can be optionally assigned to a resource

	uint32_t         m_nAllocID;     ///< Numeric ID used to track resource leaks.  This is automatically assigned by the resource constructor

	/// Global resource counter
	static uint32_t s_nAllocIDCounter;
};
