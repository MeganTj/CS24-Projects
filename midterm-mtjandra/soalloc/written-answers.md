**(a)** Explain in detail why the initial version of the small-object allocator
       provided to you exhibits this particular behavior. Aim for completeness,
       clarity and conciseness in your answer.
     
       This occurs because long-lived objects are allocated in the same chunks
       as short-lived objects. After the short-lived objects are freed from
	   a chunk, long-lived objects still remain, preventing the chunk from being
	   freed. Once a chunk is full and then some but not all objects in
	   a chunk are free'd, the next_available field of the chunk is not updated
	   to the correct next_available. It still points to the end of the chunk,
	   making the chunk seem full even though it has space.
     
     
     
     
**(b)**  Describe a modification that can be made to the internals of the
         small-object allocator to resolve this problem, so that the space
         required by the pool is no longer affected by variations in the lifetimes
         of small objects. Your description should be at a level of detail that
         another programmer could take your description and implement it against
         the code on their own.
    
        A modification is that while seraching for a nonempty chunk, if we
		notice a chunk that has its next_available pointer set to the end of the
		chunk but has free space within it, we should reset the next_available
		to the first free space within the chunk. Otherwise, the program will
		think that the chunk is full, even though it has space within it. When
		allocating a small object, we find the next available space by iterating
		through the spaces after the next_available pointer. We can check
		whether a space is free or not by having a free array within each chunk
		that keeps track of whether a space is free or occupied by an object. 
    
    
    
    
