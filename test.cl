
if (martix[idx][idy].X != 0 || martix[idx][idy].Y != 0)
{
	float dx = gridx - martix[idx][idy].X;
	float dy = gridy - martix[idx][idy].Y;
	float dis = dx * dx + dy * dy;
	float dis_sq = sqrt(dis);
	ori_power[i] = float(1.0 / dis_sq);
}
grid1[g][j].X
grid1[g][j].Y
