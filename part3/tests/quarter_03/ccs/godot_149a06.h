static Vector<int> triangulate_delaunay(const Vector<Vector2> &p_points) {
	Vector<Delaunay2D::Triangle> tr = Delaunay2D::triangulate(p_points);
	Vector<int> triangles;
	int triangle_count = tr.size();
	triangles.resize(triangle_count * 3);
	
	for (int i = 0; i < triangle_count; i++) {
		int base_idx = i * 3;
		triangles.write[base_idx] = tr[i].points[0];
		triangles.write[base_idx + 1] = tr[i].points[1];
		triangles.write[base_idx + 2] = tr[i].points[2];
	}
	return triangles;
}