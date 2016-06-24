//#pragma OPENCL EXTENSION cl_nv_printf : enable
#define NUMERSIZE1 256
typedef struct coordinationNode
{
	float X;
	float Y;
	float Z;
}  coordination;
float pow_f(float num, float m)//计算num的m次幂，num和m为单精度，num大于零 
{
	int i, j;
	float powf = 0, x, tmpm = 1;
	x = num - 1;
	for (i = 1; tmpm>1e-12 || tmpm<-1e-12; i++)//当tmpm不在次范围时，停止循环,范围可改 
	{
		for (j = 1, tmpm = 1; j <= i; j++)
			tmpm *= (m - j + 1)*x / j;
		powf += tmpm;
	}
	return powf + 1;
}
__kernel void tt(__global coordination *grid, __global coordination *martix1, int number, __global float* C)
{
	int idx = get_global_id(0);
	int idy = get_global_id(1);
	int wiWidth = get_global_size(0);
	float gridx = (grid + idx * NUMERSIZE1 + idy)->X;
	float gridy = (grid + idx * NUMERSIZE1 + idy)->Y;
	int count = 0;
	float base = 0.0;
	float temp;
	int flagx[10000];
	int flagy[10000];
	int x1 = 0, y1 = 0;
	float idp = 2;
	int tmpx, tmpy;
	float ori_power[NUMERSIZE1];
	for (int i = 0; i < NUMERSIZE1; ++i)
	{
		for (int j = 0; j < NUMERSIZE1; ++j)
		{
			if ((martix1 + i * NUMERSIZE1 + j)->X != 0 && (martix1 + i * NUMERSIZE1 + j)->Y != 0)
			{
				if ((count + 1) >= number)
				{
					break;
				}
				else
				{
					float dx = gridx - (martix1 + i * NUMERSIZE1 + j)->X;
					float dy = gridy - (martix1 + i * NUMERSIZE1 + j)->Y;
					float dis = dx * dx + dy * dy;
					float dis_sq = sqrt(dis);
					ori_power[count] = (float)(1.0 / pow_f(dis_sq, idp));
					count++;
					flagx[x1] = i;
					flagy[y1] = j;
					x1++;
					y1++;
				}
			}
		}
	}
	for (int k = 0; k < count; ++k)
	{
		base += ori_power[k];
	}
	for (int k = 0; k < count; ++k)
	{
		temp = ori_power[k] / base;
		tmpx = flagx[k];
		tmpy = flagy[k];
		(grid + idx * wiWidth + idy)->Z += temp * (martix1 + tmpx * wiWidth + tmpy)->Z;
	}
	//printf("conv is %f\n", grid[idx * wiWidth + idy].Z);
	//printf("conv is %f\n", grid[1].X);
	//C[idx * wiWidth + idy] = grid[2].X;
	C[idx * wiWidth + idy] = (grid + idx * wiWidth + idy)->Z;
}


