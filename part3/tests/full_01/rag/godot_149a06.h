static Vector<int> triangulate_delaunay(const Vector<Vector2> &p_points) {
	Vector<Delaunay2D::Triangle> tr = Delaunay2D::triangulate(p_points);
	Vector<int> triangles;
	triangles.resize(tr.size() * 3);
	
	for (int i = 0; i < tr.size(); i++) {
		int idx = i * 3;
		triangles.write[idx] = tr[i].points[0];
		triangles.write[idx + 1] = tr[i].points[1];
		triangles.write[idx + 2] = tr[i].points[2];
	}
	return triangles;
}